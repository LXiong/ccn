#include <stdio.h>

#include "api.h"
#include "timer_manager.h"

// #define TIMER_MANAGER_DEBUG

TimerManager::TimerManager(Node *node)
{
    this->node = node;
    currentMessageScheduled = NULL;
    localTimerHeap = 
        new std::priority_queue<Message*, std::vector<Message *>, MessageCompare>;
    scheduledTimerStack = new std::stack<Message*>;
}

TimerManager::~TimerManager()
{
    delete localTimerHeap;
    delete scheduledTimerStack;
}

void TimerManager::schedule(Message *msg, clocktype delay)
{
    msg->timerExpiresAt = node->getNodeTime() + delay;
    msg->timerManager = this;
    msg->isScheduledOnMainHeap = FALSE;

    /* If no message is scheduled on the main localTimerHeap, schedule this one. 
       Else check if this timer is earlier than the currently scheduled timer,
       in which case schedule this timer too.
       Otherwise, store the timer in local localTimerHeap */
    
    if(currentMessageScheduled == NULL)
    {
        currentMessageScheduled = msg;
        MESSAGE_Send(node, msg, delay);
#ifdef TIMER_MANAGER_DEBUG
        printf("TIMER: <1> %d Scheduling timer for %lf at %lf\n",
               node->nodeId,
               (double)msg->timerExpiresAt/SECOND,
               (double)node->getNodeTime()/SECOND);
#endif
    }
    else if(msg->timerExpiresAt < currentMessageScheduled->timerExpiresAt)
    {
        scheduledTimerStack->push(currentMessageScheduled);
        currentMessageScheduled = msg;
        MESSAGE_Send(node, msg, delay);
#ifdef TIMER_MANAGER_DEBUG
        printf("TIMER: <2> %d Scheduling timer for %lf at %lf\n",
               node->nodeId,
               (double)msg->timerExpiresAt/SECOND,
               (double)node->getNodeTime()/SECOND);
#endif
    }
    else
    {
        localTimerHeap->push(msg);
    }
}

void TimerManager::cancel(Message *msg)
{
    msg->cancelled = true;
}

void TimerManager::scheduleNextTimer()
{
    Message *msg;
    clocktype delay;

    while(true)
    {
        if(localTimerHeap->empty())
        {
            break;
        }

        // Get the front element of the localTimerHeap
        msg = localTimerHeap->top();

        if(!msg->cancelled)
        {
            /* 'msg' must be scheduled on the main heap */

#ifdef TIMER_MANAGER_DEBUG
            printf("TIMER: <3> %d Scheduling timer for %lf at %lf\n",
                   node->nodeId,
                   (double)msg->timerExpiresAt/SECOND,
                   (double)node->getNodeTime()/SECOND);
#endif
            if(!scheduledTimerStack->empty())
            {
                currentMessageScheduled = scheduledTimerStack->top();
                if(msg->timerExpiresAt > currentMessageScheduled->timerExpiresAt)
                {
#ifdef TIMER_MANAGER_DEBUG
                    printf("TIMER: popping %lf\n", 
                           (double)currentMessageScheduled->timerExpiresAt/SECOND);
#endif
                    scheduledTimerStack->pop();
                    return;
                }
            }

            currentMessageScheduled = msg;
            delay = msg->timerExpiresAt - node->getNodeTime();
            MESSAGE_Send(node, msg, delay);
            localTimerHeap->pop();
            return;
        }
        else
        {
            localTimerHeap->pop();
            MESSAGE_Free(node, msg);
        }
    }

    if(!scheduledTimerStack->empty())
    {
        currentMessageScheduled = scheduledTimerStack->top();
        scheduledTimerStack->pop();
    }
    else
    {
        currentMessageScheduled = NULL;
    }
}
