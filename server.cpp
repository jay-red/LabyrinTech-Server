#include "server.h"

using namespace std;

struct us_listen_socket_t* listen_socket;

Game* g = new Game;

void init_player( Player* p ) {
    set_player_uid( p, g->count );
}

short get_player_uid( Player* p ) {
    return p->uid;
}

short get_player_rotation( Player* p ) {
    return p->rotation;
}

uint32 get_player_pos_x( Player* p ) {
    return p->pos_x;
}

uint32 get_player_pos_y( Player* p ) {
    return p->pos_y;
}

char get_player_health( Player* p ) {
    return p->health;
}

bool get_player_moving( Player* p ) {
    return p->moving;
}

void set_player_uid( Player* p, short uid ) {
    p->uid = uid;
}

void set_player_rotation( Player* p, short rotation ) {
    p->rotation = rotation;
}

void set_player_pos_x( Player* p, uint32 pos_x ) {
    p->pos_x = pos_x;
}

void set_player_pos_y( Player* p, uint32 pos_y ) {
    p->pos_y = pos_y;
}

void set_player_health( Player* p, char health ) {
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
}

void on_message( uWS::WebSocket<false, true>* ws, string_view msg, uWS::OpCode opcode ) {
    LabyrinTechClient* c = ( ( LTConnData* ) ws->getUserData() )->c;
    LabyrinTechClient* p = g->head;
    char op = msg[ 0 ];
    string resp = "";
    string temp;
    switch( op ) {
        case OP_CREATE:
            g->room = c;
            g->room_created = true;
            resp.push_back( OP_CREATE );
            ws->send( resp, uWS::OpCode::TEXT );
            break;
        case OP_JOIN:
            resp.push_back( OP_JOIN );
            if( c->joined ) break;
            if( g->room_created ) {
                if( g->cdown_started || g->game_started ) resp.push_back( RESP_JOIN_STARTED );
                else if( g->count == 512 ) resp.push_back( RESP_JOIN_FULLROOM );
                else {
                    resp.push_back( RESP_JOIN_SUCCESS );
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
            } else resp.push_back( RESP_JOIN_NULLROOM );
            ws->send( resp, uWS::OpCode::TEXT );
            break;
        case OP_LEAVE:
            reset_game( g );
            break;
        case OP_CDOWN:
            resp.push_back( OP_CDOWN );
            if( g->room->ws ) g->room->ws->send( resp, uWS::OpCode::TEXT );
            resp = "           ";
            resp[ 0 ] = OP_CDOWN;
            resp[ 1 ] = ( ( char )( ( g->seed >> 24 ) & 0xFF ) );
            resp[ 2 ] = ( ( char )( ( g->seed >> 16 ) & 0xFF ) );
            resp[ 3 ] = ( ( char )( ( g->seed >> 8 ) & 0xFF ) );
            resp[ 4 ] = ( ( char )( g->seed & 0xFF ) );
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
            while( p ) {
                if( p->player && p->ws ) {
                    resp[ 5 ] = ( ( char )( ( p->player->uid >> 8 ) & 0xFF ) );
                    resp[ 6 ] = ( ( char )( p->player->uid & 0xFF ) );
                    resp[ 7 ] = ( ( char )( ( g->count >> 8 ) & 0xFF ) );
                    resp[ 8 ] = ( ( char )( g->count & 0xFF ) );
                    p->ws->send( resp, uWS::OpCode::TEXT );
                }
                p = p->next;
            }
            break;
        case OP_START:
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_POS:
            resp = "              ";
            resp[ 0 ] = OP_POS;
            resp[ 1 ] = ( char )( ( c->player->uid >> 8 ) & 0XFF );
            resp[ 2 ] = ( char )( ( c->player->uid ) & 0xFF );
            resp[ 3 ] = msg[ 1 ]; 
            resp[ 4 ] = msg[ 2 ];
            resp[ 5 ] = msg[ 3 ];
            resp[ 6 ] = msg[ 4 ];
            resp[ 7 ] = msg[ 5 ];
            resp[ 8 ] = msg[ 6 ];
            resp[ 9 ] = msg[ 7 ];
            resp[ 10 ] = msg[ 8 ];
            resp[ 11 ] = msg[ 9 ];
            resp[ 12 ] = msg[ 10 ];
            resp[ 13 ] = msg[ 11 ];
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_SMOVE:
            resp = "             ";
            resp[ 0 ] = OP_SMOVE;
            resp[ 1 ] = ( char )( ( c->player->uid >> 8 ) & 0XFF );
            resp[ 2 ] = ( char )( ( c->player->uid ) & 0xFF );
            resp[ 3 ] = msg[ 1 ];
            resp[ 4 ] = msg[ 2 ];
            resp[ 5 ] = msg[ 3 ];
            resp[ 6 ] = msg[ 4 ];
            resp[ 7 ] = msg[ 5 ];
            resp[ 8 ] = msg[ 6 ];
            resp[ 9 ] = msg[ 7 ];
            resp[ 10 ] = msg[ 8 ];
            resp[ 11 ] = msg[ 9 ];
            resp[ 12 ] = msg[ 10 ];
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_EMOVE:
            resp = "             ";
            resp[ 0 ] = OP_EMOVE;
            resp[ 1 ] = ( char )( ( c->player->uid >> 8 ) & 0XFF );
            resp[ 2 ] = ( char )( ( c->player->uid ) & 0xFF );
            resp[ 3 ] = msg[ 1 ];
            resp[ 4 ] = msg[ 2 ];
            resp[ 5 ] = msg[ 3 ];
            resp[ 6 ] = msg[ 4 ];
            resp[ 7 ] = msg[ 5 ];
            resp[ 8 ] = msg[ 6 ];
            resp[ 9 ] = msg[ 7 ];
            resp[ 10 ] = msg[ 8 ];
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_SHOOT:
            resp = "             ";
            resp[ 0 ] = OP_SHOOT;
            resp[ 1 ] = ( char )( ( c->player->uid >> 8 ) & 0XFF );
            resp[ 2 ] = ( char )( ( c->player->uid ) & 0xFF );
            resp[ 3 ] = msg[ 1 ];
            resp[ 4 ] = msg[ 2 ];
            resp[ 5 ] = msg[ 3 ];
            resp[ 6 ] = msg[ 4 ];
            resp[ 7 ] = msg[ 5 ];
            resp[ 8 ] = msg[ 6 ];
            resp[ 9 ] = msg[ 7 ];
            resp[ 10 ] = msg[ 8 ];
            resp[ 11 ] = msg[ 9 ];
            resp[ 12 ] = msg[ 10 ];
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
            break;
        case OP_HEALTH:
            resp = "             ";
            resp[ 0 ] = OP_SMOVE;
            resp[ 1 ] = ( char )( ( c->player->uid >> 8 ) & 0XFF );
            resp[ 2 ] = ( char )( ( c->player->uid ) & 0xFF );
            resp[ 3 ] = msg[ 1 ];
            ws->publish( "broadcast", msg, uWS::OpCode::TEXT );
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
