#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    values[0] = (((3*hi+mid)^0xe329)^0x360f)+2*(3*(lo-hi)+0x8217);
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = 3 * (uint32_t)((challenge >> 17) & 0xFF);
    for (size_t i = 1; i < 6;++i){
        values[i] = (((challenge>>(21+i*5))&0xF) + values[i-1]) ^ (i*9);
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t sel = (uint32_t)challenge & 7;
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = sel;
    switch(sel){
        case 0: values[1] = 3 * x + 0xc3db; break;
        case 1: values[1] = 5 * x + 0xd455; break;
        case 2: values[1] = (4 * x) ^ 0x559; break;
        case 3: values[1] = (x ^ 0x9694) + (x >> 2); break;
        case 4: values[1] = (4 * x) ^ 0xcccf; break;
        case 5: values[1] = 5 * x + 0x80cf; break;
        case 6: values[1] = 5 * x + 0x24626; break;
        case 7: values[1] = (x ^ 0x6a8) + (x >> 2); break;
    }
    return write_answer(out, cap, values, count);
}
static uint32_t p4_walk(uint32_t x,uint32_t y,uint32_t z,uint32_t w){
    uint32_t mid = y + (z - y) / 2;
    if(x == mid) return  ((mid + 0xf2) ^ w);
    else if(x < mid){
        uint32_t r = p4_walk(x,y,mid-1,w);
        return (5*r)^(w+mid)^0x1d;
    }else{
        uint32_t r = p4_walk(x,mid+1,z,w);
        return (5*r)^(w+mid)^0x1;
    }
}
int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = challenge & 0x7f;
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    
    values[0] = p4_walk(target,0,0x7f,salt); 
    return write_answer(out, cap, values, count);
}

/*
 * Phase 5 的节点表，取自炸弹 .rodata：
 *   gdb -batch -q ./bomb -ex 'x/8gx 0x405b60' -ex 'x/48wx 0x405ba0'
 * 节点按地址顺序编号，下标 = (地址 - 0x405ba0) / 16；
 * 原来的 next 是指针，这里换算成下标，-1 表示链表结束。
 */
typedef struct {
    int      next;
    uint32_t tag;
    uint32_t mask;
} Node;

static const Node p5_nodes[12] = {
    { -1, 0x02c, 0xdcac },   /* 0x405ba0 */
    {  2, 0x527, 0xf989 },   /* 0x405bb0 */
    { 10, 0x32f, 0xd5ac },   /* 0x405bc0 */
    {  1, 0x776, 0x5098 },   /* 0x405bd0 */
    {  3, 0x109, 0x91ec },   /* 0x405be0 */
    {  8, 0xb70, 0x25e4 },   /* 0x405bf0 */
    { 11, 0x917, 0xd1b3 },   /* 0x405c00 */
    {  4, 0x648, 0x564a },   /* 0x405c10 */
    {  9, 0x244, 0xdaa7 },   /* 0x405c20 */
    {  7, 0xa6c, 0xb7a3 },   /* 0x405c30 */
    {  6, 0x46e, 0x1c3d },   /* 0x405c40 */
    {  0, 0x82b, 0x40a7 },   /* 0x405c50 */
};

/* 8 个链表头（0x405b60），同样换算成节点下标；顺序已被打乱 */
static const int p5_heads[8] = { 5, 8, 9, 7, 4, 3, 1, 2 };

static uint32_t p5_traverse(uint64_t challenge, uint32_t *out) {
    int node = p5_heads[challenge & 7];
    uint32_t acc  = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t tag  = p5_nodes[node].tag;
    uint32_t step = (uint32_t)(challenge >> 3);

    acc ^= p5_nodes[node].mask;
    acc ^= 0xeff8;              /* ① 402471 那个常量 */
    *out = tag;
    acc += tag;              /* ② 加谁 */

    step &= 3;
    if (step != 0) {
        uint32_t n = 1;
        do {
            node = p5_nodes[node].next;      /* ③ 402485 */
            n += 1;
            tag = p5_nodes[node].tag;       /* ④ 40248b */
            acc ^= p5_nodes[node].mask;      /* ⑤ 40248e */
            acc += tag;      /* ⑥ 402491 */
            *out = tag;      /* ⑦ 402493 */
        } while (step >= n);
    }
    return acc;
}


int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t second = 0;

    values[0] = p5_traverse(challenge, &second);   /* 4024fc：先比返回值 */
    values[1] = second;                            /* 402504：再比 *out  */
    return write_answer(out, cap, values, count);
}

static uint32_t p6_op_0(uint32_t v) { return (v ^ (v >> 5)) + 0x7dbe; }
static uint32_t p6_op_1(uint32_t v) { return (v + 0x4f96) ^ (v >> 5); }
static uint32_t p6_op_2(uint32_t v) { return (8 * v + 0x89c7) ^ (7 * v); }
static uint32_t p6_op_3(uint32_t v) { return (v >> 1) | (v << 31); }
static uint32_t p6_op_4(uint32_t v) { return ((v << 3) | (v >> 29)) ^ 0x4800; }
static uint32_t p6_op_5(uint32_t v) { return (v ^ 0xdf18) + 2 * v; }

typedef uint32_t (*OpFn)(uint32_t);

static const OpFn p6_ops[8] = {
    p6_op_0, p6_op_4, p6_op_2, p6_op_0,
    p6_op_3, p6_op_5, p6_op_1, p6_op_1,
};

static uint32_t p6_dispatch(uint64_t challenge) {
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t a = p6_ops[challenge & 7](x);
    a ^= 0x9354;
    uint32_t b = p6_ops[(challenge >> 3) & 7](a);
    return b + 0xaa59;
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    values[0] = p6_dispatch(challenge);
    return write_answer(out, cap, values, count);
}

#define SECRET_PREFIX "0123456789abcdef0123456789abcdef"

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t lo_c = (uint32_t)challenge;
    uint32_t hi_c = (uint32_t)(challenge >> 32);
    uint32_t lo_x = (uint32_t)context;
    uint32_t hi_x = (uint32_t)(context >> 32);

    uint32_t sel = lo_x & 3;

    uint32_t esi = hi_c ^ lo_c;
    uint32_t r8  = hi_x ^ lo_x;
    uint32_t r9  = (sel * 0xb55c) ^ (uint32_t)(context >> 16);

    uint32_t edi = esi ^ r8;
    uint32_t mix = ((edi >> 3) ^ 0x67bd) + (edi ^ 0xb55c);
    uint32_t v1  = mix ^ r9;

    uint32_t edx = r8 + sel + 0x67bd + (esi >> 3);
    uint32_t v2  = v1 ^ edx;

    int n = snprintf(out, cap, SECRET_PREFIX "OPEN %u %u %u", sel, v1, v2);
    return (n < 0 || (size_t)n >= cap) ? 1 : 0;
}
