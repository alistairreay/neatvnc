/*
 * Copyright (c) 2024 wayvnc contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * VNC Authentication (RFB security type 2)
 *
 * This implements the legacy DES challenge-response authentication used by
 * macOS Screen Sharing and many other VNC clients.
 *
 * Protocol:
 *   1. Server generates 16 random bytes (challenge) and sends to client
 *   2. Client encrypts the challenge using DES with the password as key
 *   3. Server does the same encryption and compares results
 *
 * VNC DES quirk: each byte of the password has its bits reversed before
 * being used as the DES key. This is a well-known VNC protocol oddity.
 */

#include "auth/vnc-auth.h"
#include "auth/auth.h"
#include "crypto.h"
#include "stream/stream.h"
#include "common.h"
#include "neatvnc.h"

#include <string.h>
#include <nettle/des.h>

#define VNC_AUTH_CHALLENGE_LEN 16

static uint8_t reverse_bits(uint8_t b)
{
	b = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
	b = ((b & 0xcc) >> 2) | ((b & 0x33) << 2);
	b = ((b & 0xaa) >> 1) | ((b & 0x55) << 1);
	return b;
}

static void vnc_des_encrypt(uint8_t* dst, const uint8_t* challenge,
		const char* password)
{
	/* Build the 8-byte DES key from the password, bit-reversed */
	uint8_t key[8] = { 0 };
	size_t pw_len = strlen(password);
	if (pw_len > 8)
		pw_len = 8;

	for (size_t i = 0; i < pw_len; i++)
		key[i] = reverse_bits((uint8_t)password[i]);

	struct des_ctx ctx;
	des_fix_parity(8, key, key);
	des_set_key(&ctx, key);

	/* Encrypt the 16-byte challenge as two 8-byte DES ECB blocks */
	des_encrypt(&ctx, 8, dst, challenge);
	des_encrypt(&ctx, 8, dst + 8, challenge + 8);
}

int vnc_auth_send_challenge(struct nvnc_client* client)
{
	crypto_random(client->vnc_auth_challenge, VNC_AUTH_CHALLENGE_LEN);

	nvnc_log(NVNC_LOG_DEBUG, "Sending VNC Auth challenge");
	return stream_write(client->net_stream, client->vnc_auth_challenge,
			VNC_AUTH_CHALLENGE_LEN, NULL, NULL);
}

int vnc_auth_handle_response(struct nvnc_client* client)
{
	if (client->buffer_len - client->buffer_index < VNC_AUTH_CHALLENGE_LEN)
		return 0;

	const uint8_t* response = client->msg_buffer + client->buffer_index;

	struct nvnc* server = client->server;

	/* Compute expected response using the stored password */
	uint8_t expected[VNC_AUTH_CHALLENGE_LEN];
	vnc_des_encrypt(expected, client->vnc_auth_challenge,
			server->vnc_auth_password);

	if (memcmp(response, expected, VNC_AUTH_CHALLENGE_LEN) == 0) {
		security_handshake_ok(client, NULL);
		client->state = VNC_CLIENT_STATE_WAITING_FOR_INIT;
	} else {
		security_handshake_failed(client, NULL,
				"VNC authentication failed");
	}

	return VNC_AUTH_CHALLENGE_LEN;
}
