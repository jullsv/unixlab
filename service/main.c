#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <hiredis/hiredis.h>

#define TASK_QUEUE "url_shortening_tasks"
#define URL_STORAGE "shortened_urls"
#define REDIS_HOST "redis"
#define REDIS_PORT 6379

void generate_short_key(char *buffer, size_t len) {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_len = strlen(charset);
    if (len) {
        srand((unsigned int)time(NULL) * getpid());
        for (size_t n = 0; n < len - 1; n++) {
            int key = rand() % (int)charset_len;
            buffer[n] = charset[key];
        }
        buffer[len - 1] = '\0';
    }
}

int producer_mode(redisContext *c, const char *long_url) {
    char short_key[9];
    char payload[256];
    redisReply *reply;

    generate_short_key(short_key, sizeof(short_key));
    
    snprintf(payload, sizeof(payload), "%s:%s", short_key, long_url);

    reply = redisCommand(c, "LPUSH %s %s", TASK_QUEUE, payload);
    if (reply == NULL) {
        fprintf(stderr, "[PRODUCER] Error: Failed to push task to Redis.\n");
        return -1;
    }
    printf("[PRODUCER] Task sent: %s -> %s\n", short_key, long_url);
    freeReplyObject(reply);
    return 0;
}

void consumer_mode(redisContext *c) {
    redisReply *reply;
    char hostname[256];
    gethostname(hostname, 256);

    printf("[%s - WORKER] Started and listening for tasks...\n", hostname);

    while (1) {
        reply = redisCommand(c, "BRPOP %s 5", TASK_QUEUE);

        if (reply == NULL || c->err) {
            fprintf(stderr, "[%s - WORKER] Redis error: %s\n", hostname, c->errstr);
            usleep(1000000); 
            continue; 
        }

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
            char *payload = reply->element[1]->str;
            char *key = strtok(payload, ":");
            char *url = strtok(NULL, ":");
            
            if (key && url) {
                printf("[%s - WORKER] Received task: %s\n", hostname, key);
                
                usleep(500000); 

                redisCommand(c, "HSET %s %s %s", URL_STORAGE, key, url);
                
                printf("[%s - WORKER] Task finished and stored.\n", hostname);
            }
        }
        freeReplyObject(reply);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./app <role> [url]\n");
        return 1;
    }

    char *role = argv[1];
    redisContext *c;
    
    c = redisConnect(REDIS_HOST, REDIS_PORT);
    if (c == NULL || c->err) {
        if (c) {
            fprintf(stderr, "Redis connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            fprintf(stderr, "Redis connection error: Can't allocate redis context.\n");
        }
        return 1;
    }

    if (strcmp(role, "producer") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: ./app producer <url_to_shorten>\n");
            redisFree(c);
            return 1;
        }
        producer_mode(c, argv[2]);
    } else if (strcmp(role, "consumer") == 0) {
        consumer_mode(c);
    } else {
        fprintf(stderr, "Unknown role: %s. Use 'producer' or 'consumer'.\n", role);
        redisFree(c);
        return 1;
    }

    redisFree(c);
    return 0;
}
