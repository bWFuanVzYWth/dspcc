#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dspcc.h"

typedef int64_t bpidx_t;  // blueprint index

// 带子，三个输入槽 + 一个输出槽
typedef struct {
    uint64_t name;
    double x;
    double y;
    double z;
    bpidx_t i1;  // input_1
    bpidx_t i2;  // input_2
    bpidx_t i3;  // input_3
    bpidx_t o1;  // output_1
} belt_t;

typedef struct {
    bpidx_t count;
    belt_t* belt;
} building_t;

void building_init(building_t* building) {
    const size_t SIZE = 1000000;  // 默认100万建筑，通常都够用了
    building->count = 0;
    building->belt = calloc(SIZE, sizeof(belt_t));
}

void building_free(building_t* building) {
    free(building->belt);
}

double lerp(double a, double b, double k) {
    return (1.0 - k) * a + k * b;
}

void head_to_json(FILE* fp, bpidx_t building_count) {
    fprintf(fp, BLUEPRINT_JSON_HEAD, building_count);
}

void tail_to_json(FILE* fp) {
    fprintf(fp, BLUEPRINT_JSON_TAIL);
}

bpidx_t check_name(building_t* building, uint64_t name) {  // TODO 这里是不是有点慢
    belt_t* belt = building->belt;
    for (bpidx_t i = 0; i < building->count; i++) {
        if (belt[i].name == name) {
            return i;
        }
    }
    return -1;
}

int _node(building_t* building, uint64_t name, double x, double y, double z) {
    bpidx_t idx = building->count;
    belt_t* belt = building->belt;

    if (check_name(building, name) != -1) {
        printf("same name: %c\n", (char)name);
        return -1;
    } else {
        belt[idx].name = name;
        belt[idx].x = x;
        belt[idx].y = y;
        belt[idx].z = z;
        belt[idx].i1 = BLUEPRINT_NULL;
        belt[idx].i2 = BLUEPRINT_NULL;
        belt[idx].i3 = BLUEPRINT_NULL;
        belt[idx].o1 = BLUEPRINT_NULL;

        (building->count)++;

        return 0;
    }
}

int find_belt_input_slot(building_t* building, bpidx_t b) {
    belt_t* belt = building->belt;
    if (belt[b].i1 == BLUEPRINT_NULL)
        return 1;
    if (belt[b].i2 == BLUEPRINT_NULL)
        return 2;
    if (belt[b].i3 == BLUEPRINT_NULL)
        return 3;
    return -2;
}

int _connect_belt(building_t* building, uint64_t na, uint64_t nb) {
    bpidx_t a = check_name(building, na);
    bpidx_t b = check_name(building, nb);

    // 检查名字是否存在
    int name_err = 0;
    if (a == -1) {
        printf("name no found: %c\n", (char)na);
        name_err = 1;
    }
    if (b == -1) {
        printf("name no found: %c\n", (char)nb);
        name_err = 1;
    }
    if (name_err)
        return -1;

    // 检查接口数量
    belt_t* belt = building->belt;
    if (belt[a].o1 != BLUEPRINT_NULL) {
        printf("too much output: {%c} -> %c, alreay have: %c\n", (char)belt[a].name, (char)belt[b].name, (char)belt[belt[a].o1].name);
        return -1;
    }
    int slot = find_belt_input_slot(building, b);
    if (slot == -2) {
        printf("too much input: %c -> {%c}\n", (char)belt[a].name, (char)belt[b].name);
    }

    // 能走到这里说明没问题，把带子连上
    belt[a].o1 = b;
    switch (slot) {
        case 1:
            belt[b].i1 = a;
            break;
        case 2:
            belt[b].i2 = a;
            break;
        case 3:
            belt[b].i3 = a;
            break;
    }
    return 0;
}

bpidx_t get_slot(bpidx_t i, belt_t* belt_ptr) {
    if (belt_ptr->i1 == i)
        return 1;
    if (belt_ptr->i2 == i)
        return 2;
    if (belt_ptr->i3 == i)
        return 3;
    puts("error at get_solt: should not reach here");
    return 0;
}

void json_dump(FILE* fp, building_t* building) {
    bpidx_t count = building->count;
    belt_t* belt = building->belt;

    fprintf(fp, BLUEPRINT_JSON_HEAD, count);
    for (bpidx_t i = 0; i < count; i++) {
        bpidx_t slot = 0;
        if (belt[i].o1 != BLUEPRINT_NULL)
            slot = get_slot(i, &belt[belt[i].o1]);
        fprintf(fp, BELT_JSON,
                i,
                belt[i].x, belt[i].y, belt[i].z,
                belt[i].x, belt[i].y, belt[i].z,
                belt[i].o1, slot);
        if (i < count - 1)
            fprintf(fp, ",");
    }
    fprintf(fp, BLUEPRINT_JSON_TAIL);
}

int main(void) {
    building_t building;
    building_init(&building);

// 声明节点
#define new_node(name, x, y, z) _node(&building, (uint64_t)name, x, y, z)
    new_node('a', 0.0, 0.0, 0.0);
    new_node('b', 1.0, 0.0, 0.125);
    new_node('c', 1.0, 1.0, 0.25);
    new_node('d', 0.0, 1.0, 0.375);
    new_node('e', 0.0, 0.0, 0.5);

// 连接节点
#define connect(a, b) _connect_belt(&building, (uint64_t)a, (uint64_t)b)
    connect('a', 'b');
    connect('b', 'c');
    connect('c', 'd');
    connect('d', 'e');

    FILE* fp = stderr;
    json_dump(fp, &building);

    building_free(&building);

    return 0;
}