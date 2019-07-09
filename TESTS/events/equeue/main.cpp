/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "equeue.h"
#include "mbed.h"
#include "rtos.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

using namespace utest::v1;



// Test functions
void pass_func(void *eh)
{
}

void simple_func(void *p)
{
    (*(int *)p)++;
}

void sloth_func(void *p)
{
    ThisThread::sleep_for(10);
    (*(int *)p)++;
}

struct indirect {
    int *touched;
    uint8_t buffer[7];
};

void indirect_func(void *p)
{
    struct indirect *i = (struct indirect *)p;
    (*i->touched)++;
}

struct timing {
    unsigned tick;
    unsigned delay;
};

void timing_func(void *p)
{
    struct timing *timing = (struct timing *)p;
    unsigned tick = equeue_tick();

    unsigned t1 = timing->delay;
    unsigned t2 = tick - timing->tick;
    TEST_ASSERT_TRUE(t1 > t2 - 10 && t1 < t2 + 10);

    timing->tick = tick;
}

struct fragment {
    equeue_t *q;
    size_t size;
    struct timing timing;
};

void fragment_func(void *p)
{
    struct fragment *fragment = (struct fragment *)p;
    timing_func(&fragment->timing);

    struct fragment *nfragment = reinterpret_cast<struct fragment *>(equeue_alloc(fragment->q, fragment->size));
    TEST_ASSERT(nfragment);

    *nfragment = *fragment;
    equeue_event_delay(nfragment, fragment->timing.delay);

    int id = equeue_post(nfragment->q, fragment_func, nfragment);
    TEST_ASSERT(id);
}

struct cancel {
    equeue_t *q;
    int id;
};

void cancel_func(void *p)
{
    struct cancel *cancel = (struct cancel *)p;
    equeue_cancel(cancel->q, cancel->id);
}

struct nest {
    equeue_t *q;
    void (*cb)(void *);
    void *data;
};

void nest_func(void *p)
{
    struct nest *nest = (struct nest *)p;
    equeue_call(nest->q, nest->cb, nest->data);

    ThisThread::sleep_for(10);
}

void multithread_thread(void *p)
{
    equeue_t *q = reinterpret_cast<equeue_t *>(p);
    equeue_dispatch(q, -1);
}

void background_func(void *p, int ms)
{
    *(unsigned *)p = ms;
}


// Simple call tests
static void simple_call_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    equeue_call(&q, simple_func, &touched);
    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void simple_call_in_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    int id = equeue_call_in(&q, 10, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 15);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void simple_call_every_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    int id = equeue_call_every(&q, 10, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 15);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void simple_post_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int touched = false;
    struct indirect *i = reinterpret_cast<struct indirect *>(equeue_alloc(&q, sizeof(struct indirect)));
    TEST_ASSERT(i);

    i->touched = &touched;
    int id = equeue_post(&q, indirect_func, i);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(*i->touched);

    equeue_destroy(&q);
}


// Misc tests
static void destructor_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int touched;
    struct indirect *e;
    int ids[3];

    touched = 0;
    for (int i = 0; i < 3; i++) {
        e = reinterpret_cast<struct indirect *>(equeue_alloc(&q, sizeof(struct indirect)));
        TEST_ASSERT(e);

        e->touched = &touched;
        equeue_event_dtor(e, indirect_func);
        int id = equeue_post(&q, pass_func, e);
        TEST_ASSERT(id);
    }

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched == 3);

    touched = 0;
    for (int i = 0; i < 3; i++) {
        e = reinterpret_cast<struct indirect *>(equeue_alloc(&q, sizeof(struct indirect)));
        TEST_ASSERT(e);

        e->touched = &touched;
        equeue_event_dtor(e, indirect_func);
        ids[i] = equeue_post(&q, pass_func, e);
        TEST_ASSERT(ids[i]);
    }

    for (int i = 0; i < 3; i++) {
        equeue_cancel(&q, ids[i]);
    }

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched == 3);

    touched = 0;
    for (int i = 0; i < 3; i++) {
        e = reinterpret_cast<struct indirect *>(equeue_alloc(&q, sizeof(struct indirect)));
        TEST_ASSERT(e);

        e->touched = &touched;
        equeue_event_dtor(e, indirect_func);
        int id = equeue_post(&q, pass_func, e);
        TEST_ASSERT(id);
    }

    equeue_destroy(&q);
    TEST_ASSERT(touched == 3);
}

static void allocation_failure_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    void *p = equeue_alloc(&q, 4096);
    TEST_ASSERT(!p);

    for (int i = 0; i < 100; i++) {
        p = equeue_alloc(&q, 0);
    }
    TEST_ASSERT(!p);

    equeue_destroy(&q);
}

template <int N>
static void cancel_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    int *ids = reinterpret_cast<int *>(malloc(N * sizeof(int)));

    for (int i = 0; i < N; i++) {
        ids[i] = equeue_call(&q, simple_func, &touched);
    }

    for (int i = N - 1; i >= 0; i--) {
        equeue_cancel(&q, ids[i]);
    }

    free(ids);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    equeue_destroy(&q);
}

static void cancel_inflight_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;

    int id = equeue_call(&q, simple_func, &touched);
    equeue_cancel(&q, id);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    id = equeue_call(&q, simple_func, &touched);
    equeue_cancel(&q, id);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    struct cancel *cancel = reinterpret_cast<struct cancel *>(equeue_alloc(&q, sizeof(struct cancel)));
    TEST_ASSERT(cancel);
    cancel->q = &q;
    cancel->id = 0;

    id = equeue_post(&q, cancel_func, cancel);
    TEST_ASSERT(id);

    cancel->id = equeue_call(&q, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    equeue_destroy(&q);
}

static void cancel_unnecessarily_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int id = equeue_call(&q, pass_func, 0);
    for (int i = 0; i < 5; i++) {
        equeue_cancel(&q, id);
    }

    id = equeue_call(&q, pass_func, 0);
    equeue_dispatch(&q, 0);
    for (int i = 0; i < 5; i++) {
        equeue_cancel(&q, id);
    }

    bool touched = false;
    equeue_call(&q, simple_func, &touched);
    for (int i = 0; i < 5; i++) {
        equeue_cancel(&q, id);
    }

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void loop_protect_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    equeue_call_every(&q, 0, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    touched = false;
    equeue_call_every(&q, 1, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void break_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    bool touched = false;
    equeue_call_every(&q, 0, simple_func, &touched);

    equeue_break(&q);
    equeue_dispatch(&q, -1);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

static void break_no_windup_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int count = 0;
    equeue_call_every(&q, 0, simple_func, &count);

    equeue_break(&q);
    equeue_break(&q);
    equeue_dispatch(&q, -1);
    TEST_ASSERT(count == 1);

    count = 0;
    equeue_dispatch(&q, 55);
    TEST_ASSERT(count > 1);

    equeue_destroy(&q);
}

static void period_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int count = 0;
    equeue_call_every(&q, 10, simple_func, &count);

    equeue_dispatch(&q, 55);
    TEST_ASSERT(count == 5);

    equeue_destroy(&q);
}

static void nested_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int touched = 0;
    struct nest *nest = reinterpret_cast<struct nest *>(equeue_alloc(&q, sizeof(struct nest)));
    TEST_ASSERT(nest);
    nest->q = &q;
    nest->cb = simple_func;
    nest->data = &touched;

    int id = equeue_post(&q, nest_func, nest);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 5);
    TEST_ASSERT(touched == 0);

    equeue_dispatch(&q, 5);
    TEST_ASSERT(touched == 1);

    touched = 0;
    nest = reinterpret_cast<struct nest *>(equeue_alloc(&q, sizeof(struct nest)));
    TEST_ASSERT(nest);
    nest->q = &q;
    nest->cb = simple_func;
    nest->data = &touched;

    id = equeue_post(&q, nest_func, nest);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 20);
    TEST_ASSERT(touched == 1);

    equeue_destroy(&q);
}

static void sloth_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int touched = 0;
    int id = equeue_call(&q, sloth_func, &touched);
    TEST_ASSERT(id);

    id = equeue_call_in(&q, 5, simple_func, &touched);
    TEST_ASSERT(id);

    id = equeue_call_in(&q, 15, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 20);
    TEST_ASSERT(touched == 3);

    equeue_destroy(&q);
}

void multithread_test(void)
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int touched = 0;
    equeue_call_every(&q, 1, simple_func, &touched);

    Thread t1(osPriorityNormal);
    t1.start(callback(multithread_thread, &q));
    ThisThread::sleep_for(10);
    equeue_break(&q);
    err=t1.join();  // does join() return smth?
    TEST_ASSERT(!err);

    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

void background_test(void)
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    int id = equeue_call_in(&q, 20, pass_func, 0);
    TEST_ASSERT(id);

    unsigned ms;
    equeue_background(&q, background_func, &ms);
    TEST_ASSERT(ms == 20);

    id = equeue_call_in(&q, 10, pass_func, 0);
    TEST_ASSERT(id);
    TEST_ASSERT(ms == 10);

    id = equeue_call(&q, pass_func, 0);
    TEST_ASSERT(id);
    TEST_ASSERT(ms == 0);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(ms == 10);

    equeue_destroy(&q);
    TEST_ASSERT(ms == (unsigned)-1);
}


void chain_test(void)
{
    equeue_t q1;
    int err = equeue_create(&q1, 2048);
    TEST_ASSERT(!err);

    equeue_t q2;
    err = equeue_create(&q2, 2048);
    TEST_ASSERT(!err);

    equeue_chain(&q2, &q1);

    int touched = 0;

    int id1 = equeue_call_in(&q1, 20, simple_func, &touched);
    int id2 = equeue_call_in(&q2, 20, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    id1 = equeue_call(&q1, simple_func, &touched);
    id2 = equeue_call(&q2, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    id1 = equeue_call_in(&q1, 5, simple_func, &touched);
    id2 = equeue_call_in(&q2, 5, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    equeue_cancel(&q1, id1);
    equeue_cancel(&q2, id2);

    id1 = equeue_call_in(&q1, 10, simple_func, &touched);
    id2 = equeue_call_in(&q2, 10, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    equeue_dispatch(&q1, 30);

    TEST_ASSERT(touched == 6);

    equeue_destroy(&q1);
    equeue_destroy(&q2);
}

void unchain_test(void)
{
    equeue_t q1;
    int err = equeue_create(&q1, 2048);
    TEST_ASSERT(!err);

    equeue_t q2;
    err = equeue_create(&q2, 2048);
    TEST_ASSERT(!err);

    equeue_chain(&q2, &q1);

    int touched = 0;
    int id1 = equeue_call(&q1, simple_func, &touched);
    int id2 = equeue_call(&q2, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    equeue_dispatch(&q1, 0);
    TEST_ASSERT(touched == 2);

    equeue_chain(&q2, 0);
    equeue_chain(&q1, &q2);

    id1 = equeue_call(&q1, simple_func, &touched);
    id2 = equeue_call(&q2, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    equeue_dispatch(&q2, 0);
    TEST_ASSERT(touched == 4);

    equeue_destroy(&q1);
    equeue_destroy(&q2);
}

// Barrage tests
template<int N>
static void simple_barrage_test()
{
    equeue_t q;
    int err = equeue_create(&q, N * (EQUEUE_EVENT_SIZE + sizeof(struct timing)));
    TEST_ASSERT(!err);

    for (int i = 0; i < N; i++) {
        struct timing *timing = reinterpret_cast<struct timing *>(equeue_alloc(&q, sizeof(struct timing)));
        TEST_ASSERT(timing);

        timing->tick = equeue_tick();
        timing->delay = (i + 1) * 100;
        equeue_event_delay(timing, timing->delay);
        equeue_event_period(timing, timing->delay);

        int id = equeue_post(&q, timing_func, timing);
        TEST_ASSERT(id);
    }

    equeue_dispatch(&q, N * 100);

    equeue_destroy(&q);
}


template<int N>
static void fragmenting_barrage_test()
{
    equeue_t q;
    int err = equeue_create(&q,
                            2 * N * (EQUEUE_EVENT_SIZE + sizeof(struct fragment) + N * sizeof(int)));
    TEST_ASSERT(!err);

    for (int i = 0; i < N; i++) {
        size_t size = sizeof(struct fragment) + i * sizeof(int);
        struct fragment *fragment = reinterpret_cast<struct fragment *>(equeue_alloc(&q, size));
        TEST_ASSERT(fragment);

        fragment->q = &q;
        fragment->size = size;
        fragment->timing.tick = equeue_tick();
        fragment->timing.delay = (i + 1) * 100;
        equeue_event_delay(fragment, fragment->timing.delay);

        int id = equeue_post(&q, fragment_func, fragment);
        TEST_ASSERT(id);
    }

    equeue_dispatch(&q, N * 100);

    equeue_destroy(&q);
}

struct ethread {
    Thread thread;
    equeue_t *q;
    int ms;
};

static void ethread_dispatch(void *p)
{
    struct ethread *t = (struct ethread *)p;
    equeue_dispatch(t->q, t->ms);
}

template<int N>
static void multithreaded_barrage_test()
{
    equeue_t q;
    int err = equeue_create(&q, N * (EQUEUE_EVENT_SIZE + sizeof(struct timing)));
    TEST_ASSERT(!err);

    struct ethread t;
    t.q = &q;
    t.ms = N * 100;

    t.thread.start(callback(ethread_dispatch, &t));

    for (int i = 0; i < N; i++) {
        struct timing *timing = reinterpret_cast<struct timing *>(equeue_alloc(&q, sizeof(struct timing)));
        TEST_ASSERT(timing);

        timing->tick = equeue_tick();
        timing->delay = (i + 1) * 100;
        equeue_event_delay(timing, timing->delay);
        equeue_event_period(timing, timing->delay);

        int id = equeue_post(&q, timing_func, timing);
        TEST_ASSERT(id);
    }

    err = t.thread.join();
    TEST_ASSERT(!err);

    equeue_destroy(&q);
}

struct count_and_queue {
    int p;
    equeue_t *q;
};

void simple_breaker(void *p)
{
    struct count_and_queue *caq = (struct count_and_queue *)p;
    equeue_break(caq->q);
    ThisThread::sleep_for(10);
    caq->p++;
}

static void break_request_cleared_on_timeout_test()
{
    equeue_t q;
    int err = equeue_create(&q, 2048);
    TEST_ASSERT(!err);

    struct count_and_queue pq;
    pq.p = 0;
    pq.q = &q;

    int id = equeue_call_every(&q, 10, simple_breaker, &pq);

    equeue_dispatch(&q, 10);
    TEST_ASSERT(pq.p == 1);

    equeue_cancel(&q, id);

    int count = 0;
    equeue_call_every(&q, 10, simple_func, &count);

    equeue_dispatch(&q, 55);
    TEST_ASSERT(count > 1);

    equeue_destroy(&q);
}

void sibling_test(void)
{
    equeue_t q;
    int err = equeue_create(&q, 1024);
    TEST_ASSERT(!err);

    int id0 = equeue_call_in(&q, 1, pass_func, 0);
    int id1 = equeue_call_in(&q, 1, pass_func, 0);
    int id2 = equeue_call_in(&q, 1, pass_func, 0);

    struct equeue_event *e = q.queue;

    for (; e; e = e->next) {
        for (struct equeue_event *s = e->sibling; s; s = s->sibling) {
            TEST_ASSERT(!s->next);
        }
    }
    equeue_cancel(&q, id0);
    equeue_cancel(&q, id1);
    equeue_cancel(&q, id2);
    equeue_destroy(&q);
}


Case cases[] = {
    Case("simple call test", simple_call_test),
    Case("simple call in test", simple_call_in_test),
    Case("simple call every test", simple_call_every_test),
    Case("simple post test", simple_post_test),

    Case("destructor test", destructor_test),
    Case("allocation failure test", allocation_failure_test),
    Case("cancel test", cancel_test<20>),
    Case("cancel inflight test", cancel_inflight_test),
    Case("cancel unnecessarily test", cancel_unnecessarily_test),
    Case("loop protect test", loop_protect_test),
    Case("break test", break_test),
    Case("break no windup test", break_no_windup_test),
    Case("period test", period_test),
    Case("nested test", nested_test),
    Case("sloth test", sloth_test),

    Case("multithread test", multithread_test),

    Case("background test", background_test),
    Case("chain test", chain_test),
    Case("unchain test", unchain_test),

    Case("simple barrage test", simple_barrage_test<20>),
    Case("fragmenting barrage test", fragmenting_barrage_test<20>),
    Case("multithreaded barrage test", multithreaded_barrage_test<20>),
    Case("break request cleared on timeout test", break_request_cleared_on_timeout_test),
    Case("sibling test", sibling_test)

};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(20, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main()
{
    Harness::run(specification);
}
