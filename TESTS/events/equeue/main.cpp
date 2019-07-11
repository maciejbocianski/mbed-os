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

#define TEST_EQUEUE_SIZE (4*EVENTS_EVENT_SIZE)
#define TEST_THREAD_STACK_SIZE 512

// Test functions
static void pass_func(void *eh)
{
}

static void simple_func(void *p)
{
    (*(uint8_t *)p)++;
}

static void sloth_func(void *p)
{
    ThisThread::sleep_for(10);
    (*(uint8_t *)p)++;
}

struct indirect {
    uint8_t *touched;
    uint8_t buffer[7];
};

static void indirect_func(void *p)
{
    struct indirect *i = (struct indirect *)p;
    (*i->touched)++;
}

struct timing {
    unsigned tick;
    unsigned delay;
};

static void timing_func(void *p)
{
    struct timing *timing = (struct timing *)p;
    unsigned tick = equeue_tick();

    unsigned t1 = timing->delay;
    unsigned t2 = tick - timing->tick;
    //TEST_ASSERT_TRUE(t1 > t2 - 10 && t1 < t2 + 10);
    TEST_ASSERT_UINT_WITHIN(10, t2, t1);

    timing->tick = tick;
}

struct fragment {
    equeue_t *q;
    size_t size;
    struct timing timing;
};

static void fragment_func(void *p)
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

static void cancel_func(void *p)
{
    struct cancel *ccel = (struct cancel *)p;
    equeue_cancel(ccel->q, ccel->id);
}

struct nest {
    equeue_t *q;
    void (*cb)(void *);
    void *data;
};

static void nest_func(void *p)
{
    struct nest *nst = (struct nest *)p;
    equeue_call(nst->q, nst->cb, nst->data);

    ThisThread::sleep_for(10);
}

static void multithread_thread(equeue_t *p)
{
    equeue_dispatch(p, -1);
}

static void background_func(void *p, int ms)
{
    *(unsigned *)p = ms;
}


// Simple call tests

/** Test that equeue executes function passed by equeue_call.
 *
 *  Given board supports equeue.
 *  When equeue_dispatch is called.
 *  Then function passed by equeue_call is executed properly.
 *
 */
static void simple_call_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    equeue_call(&q, simple_func, &touched);
    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that equeue executes function passed by equeue_call_in.
 *
 *  Given board supports equeue.
 *  When equeue_dispatch is called.
 *  Then function passed by equeue_call_in is executed properly.
 *
 */
static void simple_call_in_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    int id = equeue_call_in(&q, 10, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 15);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that equeue executes function passed by equeue_call_every.
 *
 *  Given board supports equeue.
 *  When equeue_dispatch is called.
 *  Then function passed by equeue_call_every is executed properly.
 *
 */
static void simple_call_every_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    int id = equeue_call_every(&q, 10, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 15);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that equeue executes function passed by equeue_post.
 *
 *  Given board supports equeue.
 *  When equeue_dispatch is called.
 *  Then function passed by equeue_post is executed properly.
 *
 */
static void simple_post_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
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

/** Test that equeue executes events attached to its events destructors by equeue_event_dtor.
 *
 *  Given board supports equeue.
 *  When equeue events are being destroyed by equeue_dispatch, equeue_cancel, or equeue_destroy.
 *  Then functions attached to equeue events are executed properly.
 *
 */
static void destructor_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    struct indirect *e;
    int ids[3];

    for (int i = 0; i < 3; i++) {
        e = reinterpret_cast<struct indirect *>(equeue_alloc(&q, sizeof(struct indirect)));
        TEST_ASSERT(e);

        e->touched = &touched;
        equeue_event_dtor(e, indirect_func);
        int id = equeue_post(&q, pass_func, e);
        TEST_ASSERT(id);
    }

    equeue_dispatch(&q, 0);
    TEST_ASSERT_EQUAL_UINT8(3, touched);

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
    TEST_ASSERT_EQUAL_UINT8(3, touched);

    equeue_dispatch(&q, 0);

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
    TEST_ASSERT_EQUAL_UINT8(3, touched);
}

/** Test that equeue_alloc returns 0 when equeue can not be allocated
 *
 *  Given board supports equeue.
 *  When equeue_alloc is called and equeue can not be allocated
 *  Then function equeue_alloc returns 0.
 *
 */
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

/** Test that equeue does not execute evenets that has been canceled
 *
 *  Given board supports equeue.
 *  When events are canceled by equeue_cancel.
 *  Then they are not executed by calling equeue_dispatch.
 *
 */
template <int N>
static void cancel_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
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

/** Test that events can be cancelled by function executed by equeue_dispatch
 *
 *  Given board supports equeue.
 *  When event is cancelled by another event while dispatching.
 *  Then event that was cancelled is not being executed.
 *
 */
static void cancel_inflight_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;

    int id = equeue_call(&q, simple_func, &touched);
    equeue_cancel(&q, id);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    id = equeue_call(&q, simple_func, &touched);
    equeue_cancel(&q, id);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    struct cancel *ccel = reinterpret_cast<struct cancel *>(equeue_alloc(&q, sizeof(struct cancel)));
    TEST_ASSERT(ccel);
    ccel->q = &q;
    ccel->id = 0;

    id = equeue_post(&q, cancel_func, ccel);
    TEST_ASSERT(id);

    ccel->id = equeue_call(&q, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(!touched);

    equeue_destroy(&q);
}

/** Test that canceling event wont affect other events
 *
 *  Given board supports equeue.
 *  When event is canceled by equeue_cancel.
 *  Then other events are properly executed after calling equeue_dispatch.
 *
 */
static void cancel_unnecessarily_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
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

    uint8_t touched = 0;
    equeue_call(&q, simple_func, &touched);
    for (int i = 0; i < 5; i++) {
        equeue_cancel(&q, id);
    }

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that events that have bigger delay than the dispatch time will trigger anyway
 *
 *  Given board supports equeue.
 *  When events have bigger delay than the dispatch time.
 *  Then they will be executed anyway.
 *
 */
static void loop_protect_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    equeue_call_every(&q, 0, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    touched = 0;
    equeue_call_every(&q, 10, simple_func, &touched);

    equeue_dispatch(&q, 0);
    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that equeue_break breaks event queue out of dispatching
 *
 *  Given board supports equeue.
 *  When equeue_break is called.
 *  Then event queue will stop dispatching after finisching current dispatching cycle.
 *
 */
static void break_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    equeue_call_every(&q, 0, simple_func, &touched);

    uint8_t touched2 = 0;
    equeue_call_every(&q, 5, simple_func, &touched2);

    equeue_break(&q);
    equeue_dispatch(&q, -1);
    TEST_ASSERT(touched);
    TEST_ASSERT(!touched2);

    equeue_destroy(&q);
}

/** Test that equeue_break function breaks equeue dispatching only once
 *
 *  Given board supports equeue.
 *  When equeue_break is called several times.
 *  Then equeue is breaked only once.
 *
 */
static void break_no_windup_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    int count = 0;
    equeue_call_every(&q, 0, simple_func, &count);

    equeue_break(&q);
    equeue_break(&q);
    equeue_dispatch(&q, -1);
    TEST_ASSERT_EQUAL_INT(1, count);

    count = 0;
    equeue_dispatch(&q, 55);
    TEST_ASSERT(count > 1);

    equeue_destroy(&q);
}

/** Test that function passed by equeue_call_every is being executed periodicaly
 *
 *  Given board supports equeue.
 *  When function is passed by equeue_call_every with specified period.
 *  Then event is executed (dispatch time/period) times.
 *
 */
static void period_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    int count = 0;
    equeue_call_every(&q, 10, simple_func, &count);

    equeue_dispatch(&q, 55);
    TEST_ASSERT_EQUAL_INT(5, count);

    equeue_destroy(&q);
}

/** Test that function added to the equeue by other function which already is in equeue executes in the next dispatch, or after the end of execution of the "mother" event
 *
 *  Given board supports equeue.
 *  When nested function is added to enqueue.
 *  Then it is executed in the next dispatch, or after execution of "mother" function.
 *
 */
static void nested_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    struct nest *nst = reinterpret_cast<struct nest *>(equeue_alloc(&q, sizeof(struct nest)));
    TEST_ASSERT(nst);
    nst->q = &q;
    nst->cb = simple_func;
    nst->data = &touched;

    int id = equeue_post(&q, nest_func, nst);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 5);
    TEST_ASSERT_EQUAL_UINT8(0, touched);

    equeue_dispatch(&q, 1);
    TEST_ASSERT_EQUAL_UINT8(1, touched);

    touched = 0;
    nst = reinterpret_cast<struct nest *>(equeue_alloc(&q, sizeof(struct nest)));
    TEST_ASSERT(nst);
    nst->q = &q;
    nst->cb = simple_func;
    nst->data = &touched;

    id = equeue_post(&q, nest_func, nst);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 20);
    TEST_ASSERT_EQUAL_UINT8(1, touched);

    equeue_destroy(&q);
}

/** Test that slow functions in enqueue wont affect other functions schedule
 *
 *  Given board supports equeue.
 *  When sloth function is being called before other functions.
 *  Then all functions start according to predefinied schedule correctly.
 *
 */
static void sloth_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    int id = equeue_call(&q, sloth_func, &touched);
    TEST_ASSERT(id);

    id = equeue_call_in(&q, 5, simple_func, &touched);
    TEST_ASSERT(id);

    id = equeue_call_in(&q, 15, simple_func, &touched);
    TEST_ASSERT(id);

    equeue_dispatch(&q, 20);
    TEST_ASSERT_EQUAL_UINT8(3, touched);

    equeue_destroy(&q);
}

/** Test that equeue can be breaked of dispatching from a different thread
 *
 *  Given board supports equeue.
 *  When equeue starts dispatching in one thread.
 *  Then it can be stopped from another thread via equeue_break.
 *
 */
static void multithread_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    uint8_t touched = 0;
    equeue_call_every(&q, 1, simple_func, &touched);

    Thread t1(osPriorityNormal, TEST_THREAD_STACK_SIZE);
    t1.start(callback(multithread_thread, &q));
    ThisThread::sleep_for(10);
    equeue_break(&q);
    err = t1.join();
    TEST_ASSERT(!err);

    TEST_ASSERT(touched);

    equeue_destroy(&q);
}

/** Test that variable reffered via equeue_background shows value in ms to the next event
 *
 *  Given board supports equeue.
 *  When variable is reffered via equeue_background.
 *  Then it depicts the time in ms to the next event.
 *
 */
static void background_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    int id = equeue_call_in(&q, 20, pass_func, 0);
    TEST_ASSERT(id);

    unsigned ms;
    equeue_background(&q, background_func, &ms);
    TEST_ASSERT_EQUAL_UINT(20, ms);

    id = equeue_call_in(&q, 10, pass_func, 0);
    TEST_ASSERT(id);
    TEST_ASSERT_EQUAL_UINT(10, ms);

    id = equeue_call(&q, pass_func, 0);
    TEST_ASSERT(id);
    TEST_ASSERT_EQUAL_UINT(0, ms);

    equeue_dispatch(&q, 0);
    TEST_ASSERT_EQUAL_UINT(10, ms);

    equeue_destroy(&q);
    TEST_ASSERT_EQUAL_UINT((unsigned) - 1, ms);
}

/** Test that when chaining two equeues, by calling dispatch only on one, events are executed from both
 *
 *  Given board supports equeue.
 *  When one chained equeue is dispatched.
 *  Then events from both chained equeues are executed.
 *
 */
static void chain_test()
{
    equeue_t q1;
    int err = equeue_create(&q1, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    equeue_t q2;
    err = equeue_create(&q2, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    equeue_chain(&q2, &q1);

    uint8_t touched = 0;

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

    TEST_ASSERT_EQUAL_UINT8(6, touched);

    equeue_destroy(&q1);
    equeue_destroy(&q2);
}

/** Test that unchainig equeues makes them work on thier own
 *
 *  Given board supports equeue.
 *  When equeue is unchained.
 *  Then it can be only dispatched by calling with refference to it.
 *
 */
static void unchain_test()
{
    equeue_t q1;
    int err = equeue_create(&q1, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    equeue_t q2;
    err = equeue_create(&q2, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    equeue_chain(&q2, &q1);

    uint8_t touched = 0;
    int id1 = equeue_call(&q1, simple_func, &touched);
    int id2 = equeue_call(&q2, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    equeue_dispatch(&q1, 0);
    TEST_ASSERT_EQUAL_UINT8(2, touched);

    equeue_chain(&q2, 0);
    //equeue_chain(&q1, &q2);

    id1 = equeue_call(&q1, simple_func, &touched);
    id2 = equeue_call(&q2, simple_func, &touched);
    TEST_ASSERT(id1 && id2);

    //equeue_dispatch(&q2, 0);
    //TEST_ASSERT_EQUAL_UINT8(4, touched);
    equeue_dispatch(&q1, 0);
    TEST_ASSERT_EQUAL_UINT8(3, touched);

    equeue_destroy(&q1);
    equeue_destroy(&q2);
}

// Barrage tests

/** Test that equeue keeps good time at starting events
 *
 *  Given board supports equeue.
 *  When equeue is being dispatched.
 *  Then events happen according to the shedule with en error within a specified range.
 *
 */
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

/** Test that equeue keeps good time at starting events when events are added via functions already placed in equeue
 *
 *  Given board supports equeue.
 *  When equeue is being dispatched and new events are added via already placed in equeue.
 *  Then events happen according to the shedule with en error within a specified range.
 *
 */
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
    equeue_t *q;
    int ms;
};

static void ethread_dispatch(void *p)
{
    struct ethread *t = reinterpret_cast<struct ethread *>(p);
    equeue_dispatch(t->q, t->ms);
}

/** Test that equeue keeps good time at starting events even if it is working on different thread
 *
 *  Given board supports equeue.
 *  When equeue is being dispatched on different thread.
 *  Then events happen according to the shedule with en error within a specified range.
 *
 */
template<int N>
static void multithreaded_barrage_test()
{
    equeue_t q;
    int err = equeue_create(&q, N * (EQUEUE_EVENT_SIZE + sizeof(struct timing)));
    TEST_ASSERT(!err);

    struct ethread t;
    t.q = &q;
    t.ms = N * 100;

    Thread t1(osPriorityNormal, TEST_THREAD_STACK_SIZE);

    t1.start(callback(ethread_dispatch, &t));

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

    err = t1.join();
    TEST_ASSERT(!err);

    equeue_destroy(&q);
}

struct count_and_queue {
    int p;
    equeue_t *q;
};

static void simple_breaker(void *p)
{
    struct count_and_queue *caq = reinterpret_cast<struct count_and_queue *>(p);
    equeue_break(caq->q);
    //ThisThread::sleep_for(10);
    ThisThread::sleep_for(20);
    caq->p++;
}

/** Test that equeue stops executing events when function equeue_break is called, not when the function that called it finishes.
 *
 *  Given board supports equeue.
 *  When equeue_break is called.
 *  Then event queue will stop dispatching regardless of function that called it.
 *
 */
static void break_request_cleared_on_timeout_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
    TEST_ASSERT(!err);

    struct count_and_queue pq;
    pq.p = 0;
    pq.q = &q;

    int id = equeue_call_every(&q, 10, simple_breaker, &pq);

    //equeue_dispatch(&q, 10);
    equeue_dispatch(&q, 55);
    TEST_ASSERT_EQUAL_INT(1, pq.p);

    equeue_cancel(&q, id);

    int count = 0;
    equeue_call_every(&q, 10, simple_func, &count);

    equeue_dispatch(&q, 55);
    TEST_ASSERT(count > 1);

    equeue_destroy(&q);
}

//????
static void sibling_test()
{
    equeue_t q;
    int err = equeue_create(&q, TEST_EQUEUE_SIZE);
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
    Case("fragmenting barrage test", fragmenting_barrage_test<10>),
    Case("multithreaded barrage test", multithreaded_barrage_test<10>),
    Case("break request cleared on timeout test", break_request_cleared_on_timeout_test),
    Case("sibling test", sibling_test)

};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(40, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main()
{
    Harness::run(specification);
}
