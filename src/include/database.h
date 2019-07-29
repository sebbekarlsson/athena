#ifndef ATHENA_DATABASE_H
#define ATHENA_DATABASE_H
#include <coelum/dynamic_list.h>
#include <coelum/sprite.h>
#include <coelum/utils.h>
#include <sqlite3.h>

char* get_random_string(unsigned int length);

typedef struct DATABASE_SPRITE_STRUCT
{
    char* id;
    char* name;
    char* filepath;
    sprite_T* sprite;
} database_sprite_T;

database_sprite_T* init_database_sprite(char* id, char* name, char* filepath, sprite_T* sprite);

void database_sprite_free(database_sprite_T* database_sprite);

void database_sprite_reload_from_disk(database_sprite_T* database_sprite);

typedef struct DATABASE_ACTOR_DEFINITION_STRUCT
{
    char* id;
    char* name;
    char* sprite_id;
    char* init_script;
    char* tick_script;
    char* draw_script;
    database_sprite_T* database_sprite;

    // TODO: add friction
} database_actor_definition_T;

database_actor_definition_T* init_database_actor_definition(
    char* id,
    char* name,
    char* sprite_id,
    char* init_script,
    char* tick_script,
    char* draw_script,
    database_sprite_T* database_sprite 
);

void database_actor_definition_free(database_actor_definition_T* database_actor_definition);

typedef struct DATABASE_STRUCT
{
    const char* filename;
    sqlite3* db;
} database_T;

database_T* init_database();

sqlite3_stmt* database_exec_sql(database_T* database, char* sql, unsigned int do_error_checking);

char* database_insert_sprite(database_T* database, const char* name, sprite_T* sprite);

void database_update_sprite_name_by_id(database_T* database, const char* id, const char* name);

database_sprite_T* database_get_sprite_by_id(database_T* database, const char* id);

char* database_insert_actor_definition(
    database_T* database,
    const char* name,
    const char* sprite_id,
    const char* init_script,
    const char* tick_script,
    const char* draw_script
);

database_actor_definition_T* database_get_actor_definition_by_id(database_T* database, const char* id);

database_actor_definition_T* database_get_actor_definition_by_name(database_T* database, const char* name);

void database_update_actor_definition_by_id(
    database_T* database,
    const char* id,
    const char* name,
    const char* sprite_id,
    const char* init_script,
    const char* tick_script,
    const char* draw_script
);

typedef struct DATABASE_SCENE_STRUCT
{
    char* id;
    char* name;
    unsigned int main;

    // TODO: add bg_r, bg_g, bg_b and possibly tick_script and draw_script
} database_scene_T;

database_scene_T* init_database_scene(char* id, char* name, unsigned int main);

void database_scene_free(database_scene_T* database_scene);

database_scene_T* database_get_scene_by_id(database_T* database, const char* id);

unsigned int database_count_scenes(database_T* database);

char* database_insert_scene(database_T* database, const char* name, unsigned int main);

void database_delete_scene_by_id(database_T* database, const char* id);

void database_update_scene_by_id(database_T* database, const char* id, const char* name, unsigned int main);

dynamic_list_T* database_get_all_scenes(database_T* database);

void database_unset_main_flag_on_all_scenes(database_T* database);

typedef struct DATABASE_ACTOR_INSTANCE_STRUCT
{
    char* id;
    char* actor_definition_id;
    char* scene_id;
    float x;
    float y;
    float z;
    database_actor_definition_T* database_actor_definition;

    //TODO: add rx, ry, rz
} database_actor_instance_T;

database_actor_instance_T* init_database_actor_instance(
    char* id,
    char* actor_definition_id,
    char* scene_id,
    float x,
    float y,
    float z,
    database_actor_definition_T* database_actor_definition
);

void database_actor_instance_free(database_actor_instance_T* database_actor_instance);

char* database_insert_actor_instance(
    database_T* database,
    const char* actor_definition_id,
    const char* scene_id,
    const float x,
    const float y,
    const float z
);

dynamic_list_T* database_get_all_actor_instances_by_scene_id(database_T* database, const char* scene_id);

void database_delete_actor_instance_by_id(database_T* database, const char* id);

unsigned int database_count_actors_in_scene(database_T* database, const char* scene_id);
#endif
