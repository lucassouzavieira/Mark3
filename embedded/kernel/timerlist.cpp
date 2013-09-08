/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/
/*!

    \file   timerlist.cpp

    \brief  Timer data structure + scheduler implementations

*/

#include "kerneltypes.h"
#include "mark3cfg.h"

#include "timerlist.h"
#include "kerneltimer.h"
#include "threadport.h"
#include "kernel_debug.h"
//---------------------------------------------------------------------------
#if defined __FILE_ID__
	#undef __FILE_ID__
#endif
#define __FILE_ID__ 	TIMERLIST_CPP

#if KERNEL_USE_TIMERS

//---------------------------------------------------------------------------
/*!
    Number of ticks to account for overhead when performing Time->tick computations.
    This must be calibrated on a per-device basis.  This value is currently
    Set up for a 16-bit timer, with a 256 prescaler, 16MHz clock, on an  
    ATMega328p (i.e. ARDUINO UNO).

    !! Note - this is deprecated.  Better to have slightly long-cycled timers
    than potentially short-cycled timers.
*/
#define TL_FUDGE_FACTOR				(0)

TimerList TimerScheduler::m_clTimerList;
//---------------------------------------------------------------------------
void TimerList::Init(void)
{
    m_bTimerActive = 0;    
    m_ulNextWakeup = 0;    
}

//---------------------------------------------------------------------------
void TimerList::Add(Timer *pclListNode_)
{
    K_LONG lDelta;
	K_UCHAR bStart = 0;   
     CS_ENTER();
    
	if (GetHead() == NULL)
	{
		bStart = 1;
	}		
    
    pclListNode_->ClearNode();    
    DoubleLinkList::Add(pclListNode_);
    
    // Set the initial timer value
    pclListNode_->m_ulTimeLeft = pclListNode_->m_ulInterval;    
    
	if (!bStart)
	{
		// If the new interval is less than the amount of time remaining...
		lDelta = KernelTimer::TimeToExpiry() - pclListNode_->m_ulInterval;
    
		if (lDelta > 0)
		{
			// Set the new expiry time on the timer.
			m_ulNextWakeup = KernelTimer::SubtractExpiry((K_ULONG)lDelta);
		}    	
	}
	else	
	{
		m_ulNextWakeup = pclListNode_->m_ulInterval;
		KernelTimer::SetExpiry(m_ulNextWakeup);
		KernelTimer::Start();		
	}	
	// Set the timer as active.
	pclListNode_->m_ucFlags |= TIMERLIST_FLAG_ACTIVE;	
    CS_EXIT();
}

//---------------------------------------------------------------------------
void TimerList::Remove(Timer *pclLinkListNode_)
{
    CS_ENTER();
    
    DoubleLinkList::Remove(pclLinkListNode_);
    
	if (this->GetHead() == NULL)
	{
		KernelTimer::Stop();
	}
    
    CS_EXIT();
}

//---------------------------------------------------------------------------
void TimerList::Process(void)
{
    K_ULONG ulNewExpiry;
	K_ULONG ulOvertime;
	K_UCHAR bContinue;
	
    Timer *pclNode;
    Timer *pclPrev;
	
	// Clear the timer and its expiry time - keep it running though
	KernelTimer::ClearExpiry();
	
	do 
	{		
		ulNewExpiry = MAX_TIMER_TICKS;
		pclNode = static_cast<Timer*>(GetHead());
		pclPrev = NULL;
		bContinue = 0;
		
		// Subtract the elapsed time interval from each active timer.
		while (pclNode)
		{        
			// Active timers only...
			if (pclNode->m_ucFlags & TIMERLIST_FLAG_ACTIVE)
			{
				// Did the timer expire?
                if (pclNode->m_ulTimeLeft <= m_ulNextWakeup)
				{
					// Yes - set the "callback" flag - we'll execute the callbacks later
					pclNode->m_ucFlags |= TIMERLIST_FLAG_CALLBACK;
                                
					if (pclNode->m_ucFlags & TIMERLIST_FLAG_ONE_SHOT)
					{
						// If this was a one-shot timer, deactivate the timer.
						pclNode->m_ucFlags |= TIMERLIST_FLAG_EXPIRED;
						pclNode->m_ucFlags &= ~TIMERLIST_FLAG_ACTIVE;                    
					}
					else
					{
						// Reset the interval timer.
						//!ToDo - figure out if we need to deal with any overtime here.
						// I think we're good though...						
						pclNode->m_ulTimeLeft = pclNode->m_ulInterval;
						
                        // If the time remaining (plus the length of the tolerance interval)
                        // is less than the next expiry interval, set the next expiry interval.
                        if ((pclNode->m_ulTimeLeft + pclNode->m_ulTimerTolerance) < ulNewExpiry)
						{
                            ulNewExpiry = pclNode->m_ulTimeLeft + pclNode->m_ulTimerTolerance;
						}
					}
				}
				else
				{
					// Not expiring, but determine how K_LONG to run the next timer interval for.
					pclNode->m_ulTimeLeft -= m_ulNextWakeup;                
					if (pclNode->m_ulTimeLeft < ulNewExpiry)
					{
						ulNewExpiry = pclNode->m_ulTimeLeft;
					}
				}
			}
			pclNode = static_cast<Timer*>(pclNode->GetNext());        
		}
    
		// Process the expired timers callbacks.
		pclNode = static_cast<Timer*>(GetHead());
		while (pclNode)
		{
			pclPrev = NULL;
        
			// If the timer expired, run the callbacks now.
			if (pclNode->m_ucFlags & TIMERLIST_FLAG_CALLBACK)
			{
				// Run the callback. these callbacks must be very fast...
				pclNode->m_pfCallback( pclNode->m_pclOwner, pclNode->m_pvData );
				pclNode->m_ucFlags &= ~TIMERLIST_FLAG_CALLBACK;
			
                // If this was a one-shot timer, let's remove it.
				if (pclNode->m_ucFlags & TIMERLIST_FLAG_ONE_SHOT)
				{
					pclPrev = pclNode;
				}
			}
			pclNode = static_cast<Timer*>(pclNode->GetNext());
        
			// Remove one-shot-timers
			if (pclPrev)
			{
				Remove(pclPrev);
			}        
		}	
	
		// Check to see how much time has elapsed since the time we 
		// acknowledged the interrupt... 
		ulOvertime = KernelTimer::GetOvertime();
		
		if( ulOvertime >= ulNewExpiry ) {
			m_ulNextWakeup = ulOvertime;
			bContinue = 1;
		}
		
	// If it's taken longer to go through this loop than would take us to
	// the next expiry, re-run the timing loop
	} while (bContinue);
	
	
	// This timer elapsed, but there's nothing more to do...
	// Turn the timer off.
	if (ulNewExpiry >= MAX_TIMER_TICKS)
	{
		KernelTimer::Stop();
	}
	else 
	{
		// Update the timer with the new "Next Wakeup" value, plus whatever
		// overtime has accumulated since the last time we called this handler
		m_ulNextWakeup = KernelTimer::SetExpiry(ulNewExpiry + ulOvertime);    	
	}
}

//---------------------------------------------------------------------------
void Timer::Start( K_UCHAR bRepeat_, K_ULONG ulIntervalMs_, TimerCallback_t pfCallback_, void *pvData_ )
{
	SetIntervalMSeconds(ulIntervalMs_);
	m_pfCallback = pfCallback_;
	m_pvData = pvData_;
	if (!bRepeat_)
	{
		m_ucFlags = TIMERLIST_FLAG_ONE_SHOT;
	}
	else
	{
		m_ucFlags = 0;
	}
	m_pclOwner = Scheduler::GetCurrentThread();
	TimerScheduler::Add(this);
}

//---------------------------------------------------------------------------
void Timer::Start( K_UCHAR bRepeat_, K_ULONG ulIntervalMs_, K_ULONG ulToleranceMs_, TimerCallback_t pfCallback_, void *pvData_ )
{
    m_ulTimerTolerance = MSECONDS_TO_TICKS(ulToleranceMs_);
    Start(bRepeat_, ulIntervalMs_, pfCallback_, pvData_);
}

//---------------------------------------------------------------------------
void Timer::Stop()
{
    TimerScheduler::Remove(this);
}

//---------------------------------------------------------------------------
void Timer::SetIntervalTicks( K_ULONG ulTicks_ )
{
    m_ulInterval = ulTicks_;
}

//---------------------------------------------------------------------------
//!! The next three cost us 330 bytes of flash on AVR...
//---------------------------------------------------------------------------
void Timer::SetIntervalSeconds( K_ULONG ulSeconds_)
{
	m_ulInterval = SECONDS_TO_TICKS(ulSeconds_) - TL_FUDGE_FACTOR;
}

//---------------------------------------------------------------------------
void Timer::SetIntervalMSeconds( K_ULONG ulMSeconds_)
{
	m_ulInterval = MSECONDS_TO_TICKS(ulMSeconds_) - TL_FUDGE_FACTOR;
}

//---------------------------------------------------------------------------
void Timer::SetIntervalUSeconds( K_ULONG ulUSeconds_)
{
	m_ulInterval = USECONDS_TO_TICKS(ulUSeconds_) - TL_FUDGE_FACTOR;
}

//---------------------------------------------------------------------------
void Timer::SetTolerance(K_ULONG ulTicks_)
{
    m_ulTimerTolerance = ulTicks_;
}


#endif //KERNEL_USE_TIMERS
