#ifndef LABYRIN_TECH_HEADER
#define LABYRIN_TECH_HEADER

#include "App.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>

#define OP_CREATE   0x00
#define OP_JOIN     0x01
#define OP_LEAVE    0x02
#define OP_CDOWN    0x03
#define OP_START    0x04
#define OP_POS      0x05
#define OP_SMOVE    0x06
#define OP_EMOVE    0x07
#define OP_SHOOT    0x08
#define OP_HEALTH   0x09

#define RESP_JOIN_SUCCESS   0x00
#define RESP_JOIN_NULLROOM  0x01
#define RESP_JOIN_FULLROOM  0x02
#define RESP_JOIN_STARTED   0x03

typedef uint32_t uint32;

struct Player {
    short uid;
    short rotation;
    uint32 pos_x;
    uint32 pos_y;
    char health;
    bool moving;
};

void init_player( Player* p );

short get_player_uid( Player* p );
short get_player_rotation( Player* p );
uint32 get_player_pos_x( Player* p );
uint32 get_player_pos_y( Player* p );
char get_player_health( Player* p );
bool get_player_moving( Player* p );

void set_player_uid( Player* p, short uid );
void set_player_rotation( Player* p, short rotation );
void set_player_pos_x( Player* p, uint32 pos_x );
void set_player_pos_y( Player* p, uint32 pos_y );
void set_player_health( Player* p, char health );
void set_player_moving( Player* p, bool moving );

struct LabyrinTechClient {
    uWS::WebSocket<false, true>* ws;
    LabyrinTechClient* last;
    LabyrinTechClient* next;
    Player* player;
    bool joined;
};

void init_client( LabyrinTechClient* c );

struct LTConnData {
    LabyrinTechClient* c;
};

struct Game {
    LabyrinTechClient* room;
    LabyrinTechClient* head;
    LabyrinTechClient* tail;
    Player** players;
    uint32 seed;
    short count;
    bool room_created;
    bool cdown_started;
    bool game_started;
};

void init_game( Game* g );
void reset_game( Game* g );

void on_message( uWS::WebSocket<false, true>* ws, std::string_view msg, uWS::OpCode opcode );

#endif
