/**
 * bt-peer-protocol.h
 *
 * Copyright 2007 Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __BT_PEER_PROTOCOL_H__
#define __BT_PEER_PROTOCOL_H__

#include "bt-peer.h"

void bt_peer_send_handshake (BtPeer *peer);

void bt_peer_send_request (BtPeer *peer, guint block);

void bt_peer_choke (BtPeer *peer);
void bt_peer_unchoke (BtPeer *peer);

void bt_peer_interest (BtPeer *peer);
void bt_peer_uninterest (BtPeer *peer);

void bt_peer_data_received (BtPeer *peer, guint len, gpointer buf, gpointer data);

#endif
