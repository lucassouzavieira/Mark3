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
=========================================================================== */
/*!
    \file   autoalloc.cpp

    \brief  Automatic memory allocation for kernel objects.
*/

#include "mark3cfg.h"
#include "mark3.h"
#include "autoalloc.h"
#include "threadport.h"
#include "kernel.h"

#if KERNEL_USE_AUTO_ALLOC

//---------------------------------------------------------------------------
AutoAllocAllocator_t AutoAlloc::m_pfAllocator;
AutoAllocFree_t      AutoAlloc::m_pfFree;

//---------------------------------------------------------------------------
void AutoAlloc::Init()
{
    m_pfAllocator = NULL;
    m_pfFree = NULL;
}

//---------------------------------------------------------------------------
void* AutoAlloc::Allocate(AutoAllocType_t eType_, size_t sSize_)
{
    if (!m_pfAllocator) {
        return NULL;
    }
    return m_pfAllocator(eType_, sSize_);
}

//---------------------------------------------------------------------------
void AutoAlloc::Free(AutoAllocType_t eType_, void* pvObj_)
{
    if (!m_pfFree) {
        return;
    }
    m_pfFree(eType_, pvObj_);
}

#if KERNEL_USE_SEMAPHORE
//---------------------------------------------------------------------------
Semaphore* AutoAlloc::NewSemaphore(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_SEMAPHORE, sizeof(Semaphore));
    if (pvObj) {
        return new (pvObj) Semaphore();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroySemaphore(Semaphore *pclSemaphore_)
{
    pclSemaphore_->~Semaphore();
    Free(AUTO_ALLOC_TYPE_SEMAPHORE, pclSemaphore_);
}
#endif

#if KERNEL_USE_MUTEX
//---------------------------------------------------------------------------
Mutex* AutoAlloc::NewMutex(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_MUTEX, sizeof(Mutex));
    if (pvObj) {
        return new (pvObj) Mutex();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyMutex(Mutex *pclMutex_)
{
    pclMutex_->~Mutex();
    Free(AUTO_ALLOC_TYPE_MUTEX, pclMutex_);
}
#endif

#if KERNEL_USE_EVENTFLAG
//---------------------------------------------------------------------------
EventFlag* AutoAlloc::NewEventFlag(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_EVENTFLAG, sizeof(EventFlag));
    if (pvObj) {
        return new (pvObj) EventFlag();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyEventFlag(EventFlag *pclEventFlag_)
{
    pclEventFlag_->~EventFlag();
    Free(AUTO_ALLOC_TYPE_EVENTFLAG, pclEventFlag_);
}
#endif

#if KERNEL_USE_MESSAGE
//---------------------------------------------------------------------------
Message* AutoAlloc::NewMessage(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_MESSAGE, sizeof(Message));
    if (pvObj) {
        return new (pvObj) Message();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyMessage(Message *pclMessage_)
{
    pclMessage_->~Message();
    Free(AUTO_ALLOC_TYPE_MESSAGE, pclMessage_);
}

//---------------------------------------------------------------------------
MessageQueue* AutoAlloc::NewMessageQueue(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_MESSAGEQUEUE, sizeof(MessageQueue));
    if (pvObj) {
        return new (pvObj) MessageQueue();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyMessageQueue(MessageQueue *pclMessageQ_)
{
    pclMessageQ_->~MessageQueue();
    Free(AUTO_ALLOC_TYPE_MESSAGEQUEUE, pclMessageQ_);
}

#endif

#if KERNEL_USE_NOTIFY
//---------------------------------------------------------------------------
Notify* AutoAlloc::NewNotify(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_NOTIFY, sizeof(Notify));
    if (pvObj) {
        return new (pvObj) Notify();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyNotify(Notify *pclNotify_)
{
    pclNotify_->~Notify();
    Free(AUTO_ALLOC_TYPE_NOTIFY, pclNotify_);
}

#endif

#if KERNEL_USE_MAILBOX
//---------------------------------------------------------------------------
Mailbox* AutoAlloc::NewMailbox(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_MAILBOX, sizeof(Mailbox));
    if (pvObj) {
        return new (pvObj) Mailbox();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyMailbox(Mailbox *pclMailbox_)
{
    pclMailbox_->~Mailbox();
    Free(AUTO_ALLOC_TYPE_MAILBOX, pclMailbox_);
}

#endif

//---------------------------------------------------------------------------
Thread* AutoAlloc::NewThread(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_THREAD, sizeof(Thread));
    if (pvObj) {
        return new (pvObj) Thread();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyThread(Thread *pclThread_)
{
    pclThread_->~Thread();
    Free(AUTO_ALLOC_TYPE_THREAD, pclThread_);
}

#if KERNEL_USE_TIMERS
//---------------------------------------------------------------------------
Timer* AutoAlloc::NewTimer(void)
{
    void* pvObj = Allocate(AUTO_ALLOC_TYPE_TIMER, sizeof(Timer));
    if (pvObj) {
        return new (pvObj) Timer();
    }
    return 0;
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyTimer(Timer *pclTimer_)
{
    pclTimer_->~Timer();
    Free(AUTO_ALLOC_TYPE_TIMER, pclTimer_);
}
#endif

//---------------------------------------------------------------------------
void* AutoAlloc::NewUserTypeAllocation(uint8_t eType_)
{
    return Allocate((AutoAllocType_t)eType_, 0);
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyUserTypeAllocation(uint8_t eUserType_, void *pvObj_)
{
    Free(AUTO_ALLOC_TYPE_USER, pvObj_);
}
//---------------------------------------------------------------------------
void* AutoAlloc::NewRawData(size_t sSize_)
{
    return Allocate(AUTO_ALLOC_TYPE_RAW, sSize_);
}
//---------------------------------------------------------------------------
void AutoAlloc::DestroyRawData(void *pvData_)
{
    Free(AUTO_ALLOC_TYPE_RAW, pvData_);
}

#endif
