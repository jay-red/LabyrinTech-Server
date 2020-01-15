#include "server.h"

using namespace std;

struct us_listen_socket_t* listen_socket;

Game* g = new Game;

void init_player( Player* p ) {
    set_player_uid( p, g->count );
}

ushort get_player_uid( Player* p ) {
    return p->uid;
}

ushort get_player_rotation( Player* p ) {
    return p->rotation;
}

uint32 get_player_pos_x( Player* p ) {
    return p->pos_x;
}

uint32 get_player_pos_y( Player* p ) {
    return p->pos_y;
}

uchar get_player_health( Player* p ) {
    return p->health;
}

bool get_player_moving( Player* p ) {
    return p->moving;
}

void set_player_uid( Player* p, ushort uid ) {
    p->uid = uid;
}

void set_player_rotation( Player* p, ushort rotation ) {
    p->rotation = rotation;
}

void set_player_pos_x( Player* p, uint32 pos_x ) {
    p->pos_x = pos_x;
}

void set_player_pos_y( Player* p, uint32 pos_y ) {
    p->pos_y = pos_y;
}

void set_player_health( Player* p, uchar health ) {
    p->health = health;
}

void set_player_moving( Player* p, bool moving ) {
    p->moving = moving;
}

void init_client( LabyrinTechClient* c, uWS::WebSocket<false, true>* ws ) {
    c->last = NULL;
    c->next = NULL;
    c->ws = ws;
    c->player = NULL; 
    c->joined = false;
}

void init_game( Game* g ) {
    g->players = new Player*[512];
    g->room = NULL;
    for( int i = 0; i < 512; i++ ) {
        g->players[ i ] = NULL;
    }
}

void reset_game( Game *g ) {
    if( g->head ) {
        LabyrinTechClient* p1 = g->head;
        LabyrinTechClient* p2 = g->head;
        while( p1 ) {
            p2 = p1;
            p1 = p1->next;
            if( p2->ws ) p2->ws->close();
            delete p2;
        }
        g->head = NULL;
    }
    g->tail = NULL;
    if( g->room ) {
        if( g->room->ws ) g->room->ws->close();
        delete g->room;
        g->room = NULL;
    }
    for( int i = 0; i < 512; i++ ) {
        delete g->players[ i ];
        g->players[ i ] = NULL;
    }
    g->seed = ( uint32 ) time( 0 );
    g->count = 0;
    g->room_created = false;
    g->cdown_started = false;
    g->game_started = false;
    cout << "we resettin" << endl; 
}

void msg_push_back_bool( string* msg, bool b ) {
    ( *msg ).push_back( b == 0 ? 0 : 1 );
}

void msg_push_back_char( string* msg, uchar c ) {
    if( c < 128 ) {
        ( *msg ).push_back( c );
    } else {
        ( *msg ).push_back( ( uchar )( 0xC0 + ( ( c >> 6 ) & 0x1F ) ) );
        ( *msg ).push_back( ( uchar )( 0x80 + ( c & 0x3F ) ) );
    }
}

void msg_push_back_short( string* msg, ushort s ) {
    msg_push_back_char( msg, ( uchar )( ( s >> 8 ) & 0xFF ) );
    msg_push_back_char( msg, ( uchar )( s & 0xFF ) );
}

void msg_push_back_int( string* msg, uint32 n ) {
    msg_push_back_short( msg, ( ushort )( ( n >> 16 ) & 0xFFFF ) );
    msg_push_back_short( msg, ( ushort )( n & 0xFFFF ) );
}

void parse_msg( string* buffer, string_view* msg ) {
    uint32 s = ( *msg ).size();
    uchar b;
    for( uint32 i = 0; i < s; ++i ) {
        if( ( ( *msg )[ i ] >> 7 ) & 0x01 ) {
            b = ( uchar )( ( ( *msg )[ i ] & 0x1F ) << 6 );
            b += ( uchar )( ( *msg )[ ++i ] & 0x3F );
        } else {
            b = ( uchar )( ( *msg )[ i ] & 0x7F );
        }
        ( *buffer ).push_back( ( uchar ) b );
    }
}

void on_message( uWS::WebSocket<false, true>* ws, string_view msg, uWS::OpCode opcode ) {
    LabyrinTechClient* c = ( ( LTConnData* ) ws->getUserData() )->c;
    LabyrinTechClient* p = g->head;
    string buffer = "";
    string resp = "";
    string temp;
    parse_msg( &buffer, &msg );
    uchar op = buffer[ 0 ];
    switch( op ) {
        case OP_CREATE:
            g->room = c;
            g->room_created = true;
            msg_push_back_char( &resp, OP_CREATE );
            ws->send( resp, uWS::OpCode::TEXT );
            break;
        case OP_JOIN:
            cout << "Joined" << endl;
            msg_push_back_char( &resp, op );
            if( c->joined ) break;
            if( g->room_created ) {
                if( g->cdown_started || g->game_started ) msg_push_back_char( &resp, RESP_JOIN_STARTED );
                else if( g->count == 512 ) msg_push_back_char( &resp, RESP_JOIN_FULLROOM );
                else {
                    msg_push_back_char( &resp, RESP_JOIN_SUCCESS );
                    if( g->room->ws ) g->room->ws->send( resp, uWS::OpCode::TEXT );
                    if( g->head == NULL ) {
                        g->head = c;
                        g->tail = c;
                    } else {
                        g->tail->next = c;
                        c->last = g->tail;
                        g->tail = c;
                    }
                    ++g->count;
                    c->joined = true;
                }
            } else msg_push_back_char( &resp, RESP_JOIN_NULLROOM );
            ws->send( resp, uWS::OpCode::TEXT );
            break;
        case OP_LEAVE:
            reset_game( g );
            break;
        case OP_CDOWN:
            msg_push_back_char( &resp, OP_CDOWN );
            if( g->room->ws ) g->room->ws->send( resp, uWS::OpCode::TEXT );
            msg_push_back_int( &resp, g->seed );
            g->count = 0;
            while( p ) {
                if( p->ws && p->joined ) { 
                    p->player = new Player;
                    g->players[ g->count ] = p->player;
                    init_player( p->player );
                    ++g->count;
                }
                p = p->next;
            }
            p = g->head;
            msg_push_back_short( &resp, g->count );
            while( p ) {
                if( p->player && p->ws ) {
                    temp = resp;
                    msg_push_back_short( &temp, p->player->uid );
                    p->ws->send( temp, uWS::OpCode::TEXT );
                }
                p = p->next;
            }
            break;
        case OP_START:
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_POS:
            if( !c->player ) return;
            msg_push_back_char( &resp, op );
            msg_push_back_short( &resp, c->player->uid );
            resp.append( msg.substr( 1 ) );
            ws->publish( "broadcast", resp, uWS::OpCode::TEXT );
            break;
        case OP_SMOVE:
            if( !c->player ) return;
            msg_push_back_char( &resp, op );
            msg_push_back_short( &resp, c->player->uid );
            resp.append( msg.substr( 1 ) );
            ws->publish( "broadcast", resp, uWS::OpCode::TEXT );
            break;
        case OP_EMOVE:
            if( !c->player ) return;
            msg_push_back_char( &resp, op );
            msg_push_back_short( &resp, c->player->uid );
            resp.append( msg.substr( 1 ) );
            ws->publish( "broadcast", resp, uWS::OpCode::TEXT );
            break;
        case OP_SHOOT:
            if( !c->player ) return;
            msg_push_back_char( &resp, op );
            msg_push_back_short( &resp, c->player->uid );
            resp.append( msg.substr( 1 ) );
            ws->publish( "broadcast", resp, uWS::OpCode::TEXT );
            break;
        case OP_HEALTH:
            if( !c->player ) return;
            msg_push_back_char( &resp, op );
            msg_push_back_short( &resp, c->player->uid );
            resp.append( msg.substr( 1 ) );
            ws->publish( "broadcast", resp, uWS::OpCode::TEXT );
            break;
    }
    //ws->publish( "broadcast", msg, opcode );
}

int main() {
    init_game( g );
    reset_game( g );
    uWS::App().ws<LabyrinTechClient>( "/*", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1204,
        .open = []( auto* ws, auto* req ) {
            ws->subscribe( "broadcast" );
            LTConnData* d = ( LTConnData* )( ws->getUserData() );
            d->c = new LabyrinTechClient;
            init_client( d->c, ws );
        },
        .message = []( auto* ws, string_view msg, uWS::OpCode opcode ) {
            on_message( ws, msg, opcode );
        },
        .drain = []( auto* ws ) {
        },
        .ping = []( auto* ws ) {
        },
        .pong = []( auto* ws ) {
        },
        .close = []( auto* ws, int code, string_view message ) {
            LTConnData* d = ( LTConnData* )( ws->getUserData() );
            d->c->ws = NULL;
            if( d->c->joined ) --g->count;
            if( g->room && g->room->ws && d->c->joined ) {
                string resp = "";
                resp.push_back( OP_LEAVE );
                g->room->ws->send( resp, uWS::OpCode::TEXT );
            }
        }
    } ).listen( stoi( getenv( "PORT" ) ), []( auto* token ) {
        listen_socket = token;
        if( token ) {
            std::cout << "Listening on port " << getenv( "PORT" ) << std::endl;
        }
    } ).run();
}
