#include "include/database.h"
#include <coelum/file_utils.h>
#include <string.h>
#include <coelum/actor.h>
#include <spr/spr.h>
#include <coelum/textures.h>


/**
 * Get a random string with specified length
 *
 * @deprecated
 *
 * @parma unsigned int length
 *
 * @return char*
 */
char* get_random_string(unsigned int length)
{
    init_random();

    char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* string = calloc(length + 1, sizeof(char));
    string[0] = '\0';

    for (int i = 0; i < length; i++)
    {
        char* str = calloc(2, sizeof(char));
        str[0] = chars[random_int(0, strlen(chars))];
        strcat(string, str);
        free(str);
    }

    return string;
}

database_T* init_database()
{
    database_T* database = calloc(1, sizeof(struct DATABASE_STRUCT));
    database->filename = "application.db";

    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open(database->filename, &db);
    
    if (rc != SQLITE_OK)
    {
        
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return database;
    }

    char *sql = "CREATE TABLE IF NOT EXISTS actor_definitions(id TEXT, name TEXT, init_script TEXT, tick_script TEXT, draw_script TEXT, sprite_id TEXT);"
                "CREATE TABLE IF NOT EXISTS actor_instances(id TEXT, actor_definition_id TEXT, x FLOAT, y FLOAT, z FLOAT, scene_id TEXT);"
                "CREATE TABLE IF NOT EXISTS sprites(id TEXT, name TEXT, filepath TEXT);"
                "CREATE TABLE IF NOT EXISTS scenes(id TEXT, name TEXT, bg_r INT, bg_g INT, bg_b INT, main INT)";
    
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK)
    {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return database;
    } 
    
    sqlite3_close(db);

    return database;
}

database_sprite_T* init_database_sprite(char* id, char* name, char* filepath, sprite_T* sprite)
{
    database_sprite_T* database_sprite = calloc(1, sizeof(struct DATABASE_SPRITE_STRUCT));
    database_sprite->id = id;
    database_sprite->name = name; 
    database_sprite->filepath = filepath;
    database_sprite->sprite = sprite;

    return database_sprite;
}

void database_sprite_free(database_sprite_T* database_sprite)
{
    sprite_free(database_sprite->sprite);
    free(database_sprite);
}

void database_sprite_reload_from_disk(database_sprite_T* database_sprite)
{
    printf(
        "Reloading sprite %s from file %s...\n",
        database_sprite->name,
        database_sprite->filepath
    );

    if (database_sprite->sprite)
        sprite_free(database_sprite->sprite);

    database_sprite->sprite = load_sprite_from_disk(database_sprite->filepath);
}

database_actor_definition_T* init_database_actor_definition(
    char* id,
    char* name,
    char* sprite_id,
    char* init_script,
    char* tick_script,
    char* draw_script,
    database_sprite_T* database_sprite
)
{
    database_actor_definition_T* database_actor_definition = calloc(
        1,
        sizeof(struct DATABASE_ACTOR_DEFINITION_STRUCT)        
    );
    database_actor_definition->id = id;
    database_actor_definition->name = name;
    database_actor_definition->sprite_id = sprite_id;
    database_actor_definition->init_script = init_script;
    database_actor_definition->tick_script = tick_script;
    database_actor_definition->draw_script = draw_script;
    database_actor_definition->database_sprite = database_sprite;

    return database_actor_definition;
}

void database_actor_definition_free(database_actor_definition_T* database_actor_definition)
{
    free(database_actor_definition->id);
    free(database_actor_definition->sprite_id);
    free(database_actor_definition->init_script);
    free(database_actor_definition->tick_script);
    free(database_actor_definition->draw_script);
    database_sprite_free(database_actor_definition->database_sprite);
    free(database_actor_definition);
}

sqlite3_stmt* database_exec_sql(database_T* database, char* sql, unsigned int do_error_checking)
{
    sqlite3* db = database->db;
	sqlite3_stmt* stmt;
	
	sqlite3_open(database->filename, &db);

	if (db == NULL)
	{
		printf("Failed to open DB\n");
		return (void*) 0;
	}

	printf("Performing query...\n");
    printf("%s\n", sql);
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (do_error_checking)
    {
        int rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            printf("ERROR executing query: %s\n", sqlite3_errmsg(db));
            return (void*) 0;
        }
    }

	return stmt;
}

char* database_insert_sprite(database_T* database, const char* name, sprite_T* sprite)
{
    char* id = get_random_string(16);
    char* sql_template = "INSERT INTO sprites VALUES(\"%s\", \"%s\", \"%s\")";
    char* sql = calloc(300, sizeof(char));

    char* filepath = calloc(strlen("sprites/") + strlen(name) + strlen(".bin"), sizeof(char));
    sprintf(filepath, "sprites/%s.spr", name);

    sprintf(sql, sql_template, id, name, filepath);
    printf("%s\n", sql);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 1);
    sqlite3_finalize(stmt);
    sqlite3_close(database->db);
    free(sql);

    spr_frame_T** frames = (void*) 0;
    size_t frames_size = 0;

    for (int i = 0; i < sprite->textures->size; i++)
    {
        texture_T* texture = (texture_T*) sprite->textures->items[i];
        spr_frame_T* frame = spr_init_frame_from_data(texture->data, texture->width, texture->height);
        frames_size += 1;
        frames = realloc(frames, frames_size * sizeof(struct SPR_FRAME_STRUCT*));
        frames[frames_size-1] = frame;
    }

    spr_T* spr = init_spr(
        sprite->width,
        sprite->height,
        255,
        255,
        255,
        sprite->frame_delay,
        sprite->animate,
        frames,
        frames_size
    );

    spr_write_to_file(spr, filepath);

    spr_free(spr);

    return id;
}

void database_update_sprite_name_by_id(database_T* database, const char* id, const char* name)
{
    char* sql_template = "UPDATE sprites SET name=\"%s\" WHERE id=\"%s\"";
    char* sql = calloc(strlen(sql_template) + strlen(id) + strlen(name) + 1, sizeof(char));
    sprintf(sql, sql_template, name, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        printf("ERROR executing query: %s\n", sqlite3_errmsg(database->db));
    }

	sqlite3_finalize(stmt);
	sqlite3_close(database->db);
}

database_sprite_T* database_get_sprite_by_id(database_T* database, const char* id)
{
    char* sql_template = "SELECT * FROM sprites WHERE id=\"%s\" LIMIT 1";
    char* sql = calloc(strlen(sql_template) + strlen(id) + 1, sizeof(char));
    sprintf(sql, sql_template, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);
    free(sql);

    const unsigned char* name = 0;
    const unsigned char* filepath = 0;
    sprite_T* sprite = (void*) 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        name = sqlite3_column_text(stmt, 1);
        filepath = sqlite3_column_text(stmt, 2);

        if (filepath)
            sprite = load_sprite_from_disk(filepath);
	}	

    char* id_new = calloc(strlen(id) + 1, sizeof(char));
    strcpy(id_new, id);

    char* name_new = calloc(strlen(name) + 1, sizeof(char));
    strcpy(name_new, name);

    char* filepath_new = calloc(strlen(filepath) + 1, sizeof(char));
    strcpy(filepath_new, filepath);

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return init_database_sprite(id_new, name_new, filepath_new, sprite);
}

char* database_insert_actor_definition(
    database_T* database,
    const char* name,
    const char* sprite_id,
    const char* init_script,
    const char* tick_script,
    const char* draw_script
)
{
    char* id = get_random_string(16);
    char* sql_template = "INSERT INTO actor_definitions VALUES(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")";
    char* sql = calloc(600, sizeof(char));

    sprintf(sql, sql_template, id, name, init_script, tick_script, draw_script, sprite_id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 1);
    sqlite3_finalize(stmt);
    sqlite3_close(database->db);
    free(sql);

    return id;
}

database_actor_definition_T* database_get_actor_definition_by_id(database_T* database, const char* id)
{
    char* sql_template = "SELECT * FROM actor_definitions WHERE id=\"%s\" LIMIT 1";
    char* sql = calloc(strlen(sql_template) + strlen(id) + 1, sizeof(char));
    sprintf(sql, sql_template, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);
    free(sql);

    const unsigned char* name = 0;
    const unsigned char* sprite_id = 0;
    const unsigned char* init_script = 0;
    const unsigned char* tick_script = 0;
    const unsigned char* draw_script = 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        name = sqlite3_column_text(stmt, 1);
        init_script = sqlite3_column_text(stmt, 2);
        tick_script = sqlite3_column_text(stmt, 3);
        draw_script = sqlite3_column_text(stmt, 4);
        sprite_id = sqlite3_column_text(stmt, 5);
	}	

    char* id_new = calloc(strlen(id) + 1, sizeof(char));
    strcpy(id_new, id);
    
    char* name_new = calloc(strlen(name) + 1, sizeof(char));
    strcpy(name_new, name);

    char* sprite_id_new = calloc(strlen(sprite_id) + 1, sizeof(char));
    strcpy(sprite_id_new, sprite_id);

    char* init_script_new = calloc(strlen(init_script) + 1, sizeof(char));
    strcpy(init_script_new, init_script);
    
    char* tick_script_new = calloc(strlen(tick_script) + 1, sizeof(char));
    strcpy(tick_script_new, tick_script);

    char* draw_script_new = calloc(strlen(draw_script) + 1, sizeof(char));
    strcpy(draw_script_new, draw_script);

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return init_database_actor_definition(
        id_new,
        name_new,
        sprite_id_new,
        init_script_new,
        tick_script_new,
        draw_script_new,
        database_get_sprite_by_id(database, sprite_id_new)
    );
}

database_actor_definition_T* database_get_actor_definition_by_name(database_T* database, const char* name)
{
    char* sql_template = "SELECT * FROM actor_definitions WHERE name=\"%s\" LIMIT 1";
    char* sql = calloc(strlen(sql_template) + strlen(name) + 1, sizeof(char));
    sprintf(sql, sql_template, name);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);
    free(sql);

    const unsigned char* id = 0;
    const unsigned char* sprite_id = 0;
    const unsigned char* init_script = 0;
    const unsigned char* tick_script = 0;
    const unsigned char* draw_script = 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        id = sqlite3_column_text(stmt, 0);
        init_script = sqlite3_column_text(stmt, 2);
        tick_script = sqlite3_column_text(stmt, 3);
        draw_script = sqlite3_column_text(stmt, 4);
        sprite_id = sqlite3_column_text(stmt, 5);
	}	

    char* id_new = calloc(strlen(id) + 1, sizeof(char));
    strcpy(id_new, id);
    
    char* name_new = calloc(strlen(name) + 1, sizeof(char));
    strcpy(name_new, name);

    char* sprite_id_new = calloc(strlen(sprite_id) + 1, sizeof(char));
    strcpy(sprite_id_new, sprite_id);

    char* init_script_new = calloc(strlen(init_script) + 1, sizeof(char));
    strcpy(init_script_new, tick_script);

    char* tick_script_new = calloc(strlen(tick_script) + 1, sizeof(char));
    strcpy(tick_script_new, tick_script);

    char* draw_script_new = calloc(strlen(draw_script) + 1, sizeof(char));
    strcpy(draw_script_new, draw_script);

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return init_database_actor_definition(
        id_new,
        name_new,
        sprite_id_new,
        init_script_new,
        tick_script_new,
        draw_script_new,
        database_get_sprite_by_id(database, sprite_id_new)
    );
}

void database_update_actor_definition_by_id(
    database_T* database,
    const char* id,
    const char* name,
    const char* sprite_id,
    const char* init_script,
    const char* tick_script,
    const char* draw_script
)
{
    char* sql_template = 
        "UPDATE actor_definitions SET name=\"%s\","
        " sprite_id=\"%s\","
        " init_script=\"%s\","
        " tick_script=\"%s\","
        " draw_script=\"%s\""
        " WHERE id=\"%s\";";

    char* sql = calloc(
        strlen(sql_template) + strlen(id) + strlen(name) + strlen(init_script) + strlen(tick_script) + strlen(draw_script) + 128,
        sizeof(char)
    );

    sprintf(sql, sql_template, name, sprite_id, init_script, tick_script, draw_script, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        printf("ERROR executing query: %s\n", sqlite3_errmsg(database->db));
    }

	sqlite3_finalize(stmt);
	sqlite3_close(database->db);
    free(sql);
}

database_scene_T* init_database_scene(char* id, char* name, unsigned int main)
{
    database_scene_T* database_scene = calloc(1, sizeof(struct DATABASE_SCENE_STRUCT));
    database_scene->id = id;
    database_scene->name = name;
    database_scene->main = main;

    return database_scene;
}

void database_scene_free(database_scene_T* database_scene)
{
    free(database_scene->id);
    free(database_scene->name);
    free(database_scene);
}

database_scene_T* database_get_scene_by_id(database_T* database, const char* id)
{
    char* sql_template = "SELECT * FROM scenes WHERE id=\"%s\" LIMIT 1";
    char* sql = calloc(strlen(sql_template) + strlen(id) + 1, sizeof(char));
    sprintf(sql, sql_template, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);
    free(sql);

    const unsigned char* name = 0;
    unsigned int main = 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        name = sqlite3_column_text(stmt, 1);
        main = sqlite3_column_int(stmt, 5);
	}	

    char* id_new = calloc(strlen(id) + 1, sizeof(char));
    strcpy(id_new, id);
    
    char* name_new = calloc(strlen(name) + 1, sizeof(char));
    strcpy(name_new, name);

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return init_database_scene(
        id_new,
        name_new,
        main
    );
}

unsigned int database_count_scenes(database_T* database)
{
    const char* sql = "SELECT count(*) FROM scenes;";

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    unsigned int count = 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return count;
}

char* database_insert_scene(database_T* database, const char* name, unsigned int main)
{
    char* id = get_random_string(16);
    char* sql_template = "INSERT INTO scenes VALUES(\"%s\", \"%s\", 255, 255, 255, %d)";
    char* sql = calloc(400, sizeof(char));

    sprintf(sql, sql_template, id, name, main);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 1);
    sqlite3_finalize(stmt);
    sqlite3_close(database->db);
    free(sql);

    return id;
}

void database_delete_scene_by_id(database_T* database, const char* id)
{
    char* sql_template = "DELETE * FROM scenes WHERE id=\"%s\"";
    char* sql = calloc(strlen(sql_template) + strlen(id) + 1, sizeof(char));

    sprintf(sql, sql_template, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 1);
    sqlite3_finalize(stmt);
    sqlite3_close(database->db);
    free(sql);
}

void database_update_scene_by_id(database_T* database, const char* id, const char* name, unsigned int main)
{
    char* sql_template = "UPDATE scenes SET name=\"%s\", main=%d WHERE id=\"%s\"";
    char* sql = calloc(strlen(sql_template) + strlen(id) + strlen(name) + 128, sizeof(char));
    sprintf(sql, sql_template, name, main, id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        printf("ERROR executing query: %s\n", sqlite3_errmsg(database->db));
    }

	sqlite3_finalize(stmt);
	sqlite3_close(database->db);
    free(sql);
}

dynamic_list_T* database_get_all_scenes(database_T* database)
{
    dynamic_list_T* database_scenes = init_dynamic_list(sizeof(struct DATABASE_SCENE_STRUCT*));

    char* sql = "SELECT * FROM scenes ORDER BY main DESC";

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
        const char* id = sqlite3_column_text(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);;
        unsigned int main = sqlite3_column_int(stmt, 5);

        char* id_new = calloc(strlen(id) + 1, sizeof(char));
        strcpy(id_new, id);
        
        char* name_new = calloc(strlen(name) + 1, sizeof(char));
        strcpy(name_new, name);

        database_scene_T* database_scene = init_database_scene(
            id_new,
            name_new,
            main            
        );

        dynamic_list_append(database_scenes, database_scene);
	}

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return database_scenes;
}

void database_unset_main_flag_on_all_scenes(database_T* database)
{
    char* sql = "UPDATE scenes SET main=0";
    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        printf("ERROR executing query: %s\n", sqlite3_errmsg(database->db));
    }

	sqlite3_finalize(stmt);
	sqlite3_close(database->db);
}

database_actor_instance_T* init_database_actor_instance(
    char* id,
    char* actor_definition_id,
    char* scene_id,
    float x,
    float y,
    float z,
    database_actor_definition_T* database_actor_definition
)
{
    database_actor_instance_T* database_actor_instance = calloc(1, sizeof(struct DATABASE_ACTOR_INSTANCE_STRUCT));
    database_actor_instance->id = id;
    database_actor_instance->actor_definition_id = actor_definition_id;
    database_actor_instance->scene_id = scene_id;
    database_actor_instance->x = x;
    database_actor_instance->y = y;
    database_actor_instance->z = z;
    database_actor_instance->database_actor_definition = database_actor_definition;

    return database_actor_instance;
}

void database_actor_instance_free(database_actor_instance_T* database_actor_instance)
{
    free(database_actor_instance->id);
    free(database_actor_instance->actor_definition_id);
    free(database_actor_instance->scene_id);
    database_actor_definition_free(database_actor_instance->database_actor_definition);
    free(database_actor_instance);
}

char* database_insert_actor_instance(
    database_T* database,
    const char* actor_definition_id,
    const char* scene_id,
    const float x,
    const float y,
    const float z
)
{
    char* id = get_random_string(16);
    char* sql_template = "INSERT INTO actor_instances VALUES(\"%s\", \"%s\", %12.6f, %12.6f, %12.6f, \"%s\")";
    char* sql = calloc(400, sizeof(char));

    sprintf(sql, sql_template, id, actor_definition_id, x, y, z, scene_id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 1);
    sqlite3_finalize(stmt);
    sqlite3_close(database->db);
    free(sql);

    return id;
}

dynamic_list_T* database_get_all_actor_instances_by_scene_id(database_T* database, const char* scene_id)
{
    dynamic_list_T* database_actor_instances = init_dynamic_list(sizeof(struct DATABASE_ACTOR_INSTANCE_STRUCT*));
    
    //"CREATE TABLE IF NOT EXISTS actor_instances(id TEXT, actor_definition_id TEXT, x FLOAT, y FLOAT, z FLOAT, scene_id TEXT);"

    char* sql_template = "SELECT * FROM actor_instances WHERE scene_id=\"%s\"";
    char* sql = calloc(strlen(sql_template) + strlen(scene_id) + 1, sizeof(char));
    sprintf(sql, sql_template, scene_id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
        const unsigned char* id = sqlite3_column_text(stmt, 0);
        const unsigned char* actor_definition_id = sqlite3_column_text(stmt, 1);
        const float x = sqlite3_column_double(stmt, 2);
        const float y = sqlite3_column_double(stmt, 3);
        const float z = sqlite3_column_double(stmt, 4);

        char* id_new = calloc(strlen(id) + 1, sizeof(char));
        strcpy(id_new, id);
        
        char* actor_definition_id_new = calloc(strlen(actor_definition_id) + 1, sizeof(char));
        strcpy(actor_definition_id_new, actor_definition_id);

        char* scene_id_new = calloc(strlen(scene_id) + 1, sizeof(char));
        strcpy(scene_id_new, scene_id);

        database_actor_instance_T* database_actor_instance = init_database_actor_instance(
            id_new,
            actor_definition_id_new,
            scene_id_new,
            x,
            y,
            z,
            database_get_actor_definition_by_id(database, actor_definition_id)
        );

        dynamic_list_append(database_actor_instances, database_actor_instance);
	}

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    return database_actor_instances;
}

unsigned int database_count_actors_in_scene(database_T* database, const char* scene_id)
{
    const char* sql_template = "SELECT count(*) FROM actor_instances WHERE scene_id=\"%s\"";
    char* sql = calloc(1, (strlen(sql_template) + strlen(scene_id) + 1) * sizeof(char));

    sprintf(sql, sql_template, scene_id);

    sqlite3_stmt* stmt = database_exec_sql(database, sql, 0);

    unsigned int count = 0;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
	sqlite3_close(database->db);

    free(sql);

    return count;
}
