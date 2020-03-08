#include <multiJob.h>


void multiJob::start()
{
   setState(multiJob_RUNNING);
   run();
   if(!(state() & multiJob_CANCEL))
   {
      setState(multiJob_FINISHED);
   }
}

void multiJob::setState(int value, bool on)
{
   std::shared_ptr<multiJob> thisShared = getSharedFromThis();

   // we will need to make sure that the state flags are set properly
   // so if you turn on running then you can't have finished or ready turned onturned on
   // but can stil have cancel turned on
   //
   int newState = m_state;
   if(on)
   {
      newState = ((newState | value)&multiJob_ALL);
   }
   else 
   {
      newState = ((newState & ~value)&multiJob_ALL);
   }

   int oldState     = 0;
   int currentState = 0;
   std::shared_ptr<multiJobCallback> callback;

   bool stateChangedFlag = false;
   {
      std::lock_guard<std::mutex> lock(m_jobMutex);
      
      stateChangedFlag = newState != m_state;
      oldState = m_state;
      m_state = static_cast<State>(newState);
      currentState = m_state;
      callback = m_callback;
   }
   
   if(stateChangedFlag&&callback)
   {
      if(!(oldState&multiJob_READY)&&
         (currentState&multiJob_READY))
      {
         callback->ready(thisShared);
      }
      else if(!(oldState&multiJob_RUNNING)&&
              (currentState&multiJob_RUNNING))
      {
         callback->started(thisShared);
      }
      else if(!(oldState&multiJob_CANCEL)&&
              (currentState&multiJob_CANCEL))
      {
         callback->canceled(thisShared);
      }
      else if(!(oldState&multiJob_FINISHED)&&
              (currentState&multiJob_FINISHED))
      {
         callback->finished(thisShared);
      }
   }
}

