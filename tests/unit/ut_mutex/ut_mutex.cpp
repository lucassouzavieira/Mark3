/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 - 2017 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/

//---------------------------------------------------------------------------

#include "kerneltypes.h"
#include "kernel.h"
#include "../ut_platform.h"
#include "thread.h"
#include "mutex.h"

//===========================================================================
// Local Defines
//===========================================================================
#define MUTEX_STACK_SIZE (256)
static K_WORD aucTestStack[MUTEX_STACK_SIZE];
static Thread clMutexThread;

static K_WORD           aucTestStack2[MUTEX_STACK_SIZE];
static Thread           clTestThread2;
static volatile uint8_t u8Token;

//===========================================================================
// Define Test Cases Here
//===========================================================================

void TypicalMutexTest(void* mutex_)
{
    Mutex* pclMutex = (Mutex*)mutex_;

    pclMutex->Claim();
    u8Token = 0x69;
    pclMutex->Release();

    // Exit the thread when we're done this operation.
    Scheduler::GetCurrentThread()->Exit();
}

TEST(ut_typical_mutex)
{
    // Test - Typical mutex usage, ensure that two threads can synchronize
    // access to a single resource
    Mutex clMutex;

    clMutex.Init();

    // Create a higher-priority thread that will immediately pre-empt u16.
    // Verify that while we have the mutex held, that the high-priority thread
    // is blocked waiting for u16 to relinquish access.
    clMutexThread.Init(aucTestStack, MUTEX_STACK_SIZE, 7, TypicalMutexTest, (void*)&clMutex);

    clMutex.Claim();

    u8Token = 0x96;
    clMutexThread.Start();

    // Spend some time sleeping, just to drive the point home...
    Thread::Sleep(100);

    // Test Point - Verify that the token value hasn't changed (which would
    // indicate the high-priority thread held the mutex...)
    EXPECT_EQUALS(u8Token, 0x96);

    // Relese the mutex, see what happens.
    clMutex.Release();

    // Test Point - Verify that after releasing the mutex, the higher-priority
    // thread immediately resumes, claiming the mutex, and adjusting the
    // token value to its value.  Check the new token value here.
    EXPECT_EQUALS(u8Token, 0x69);
}
TEST_END

//===========================================================================
void TimedMutexTest(void* mutex_)
{
    Mutex* pclMutex = (Mutex*)mutex_;

    pclMutex->Claim();
    Thread::Sleep(20);
    pclMutex->Release();

    Scheduler::GetCurrentThread()->Exit();
}

//===========================================================================
TEST(ut_timed_mutex)
{
    // Test - Enusre that when a thread fails to obtain a resource in a
    // timeout scenario, that the timeout is reported correctly

    Mutex clMutex;
    clMutex.Init();

    clMutexThread.Init(aucTestStack, MUTEX_STACK_SIZE, 7, TimedMutexTest, (void*)&clMutex);
    clMutexThread.Start();

    EXPECT_FALSE(clMutex.Claim(10));

    Thread::Sleep(20);

    clMutexThread.Init(aucTestStack, MUTEX_STACK_SIZE, 7, TimedMutexTest, (void*)&clMutex);
    clMutexThread.Start();

    EXPECT_TRUE(clMutex.Claim(30));
}
TEST_END

//===========================================================================
void LowPriThread(void* mutex_)
{
    Mutex* pclMutex = (Mutex*)mutex_;

    pclMutex->Claim();

    Thread::Sleep(100);

    pclMutex->Release();

    while (1) {
        Thread::Sleep(1000);
    }
}

//===========================================================================
void HighPriThread(void* mutex_)
{
    Mutex* pclMutex = (Mutex*)mutex_;

    pclMutex->Claim();

    Thread::Sleep(100);

    pclMutex->Release();

    while (1) {
        Thread::Sleep(1000);
    }
}

//===========================================================================
TEST(ut_priority_mutex)
{
    // Test - Priority inheritence protocol.  Ensure that the priority
    // inversion problem is correctly avoided by our semaphore implementation
    // In the low/med/high scenario, we play the "med" priority thread
    Mutex clMutex;
    clMutex.Init();

    Scheduler::GetCurrentThread()->SetPriority(3);

    clMutexThread.Init(aucTestStack, MUTEX_STACK_SIZE, 2, LowPriThread, (void*)&clMutex);
    clTestThread2.Init(aucTestStack2, MUTEX_STACK_SIZE, 4, HighPriThread, (void*)&clMutex);

    // Start the low-priority thread and give it the mutex
    clMutexThread.Start();
    Thread::Sleep(20);

    // Start the high-priority thread, which will block, waiting for the
    // low-priority action to complete...
    clTestThread2.Start();
    Thread::Sleep(20);

    // Test point - Low-priority thread boost:
    // Check the priorities of the threads.  The low-priority thread
    // should now have the same priority as the high-priority thread
    EXPECT_EQUALS(clMutexThread.GetCurPriority(), 4);
    EXPECT_EQUALS(clTestThread2.GetCurPriority(), 4);

    Thread::Sleep(2000);

    // Test point - Low-priority thread drop:
    // After the threads have relinquished their mutexes, ensure that
    // they are placed back at their correct priorities

    EXPECT_EQUALS(clMutexThread.GetCurPriority(), 2);
    EXPECT_EQUALS(clTestThread2.GetCurPriority(), 4);

    clMutexThread.Exit();
    clTestThread2.Exit();
}
TEST_END

//===========================================================================
// Test Whitelist Goes Here
//===========================================================================
TEST_CASE_START
TEST_CASE(ut_typical_mutex), TEST_CASE(ut_timed_mutex), TEST_CASE(ut_priority_mutex), TEST_CASE_END
