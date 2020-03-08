#include <Thread.h>

/**
* Barrier is a class used to block threads so we can synchronize and entry point.
**/

multi::Barrier::Barrier(int maxCount)
: m_maxCount(maxCount),
  m_blockedCount(0),
  m_waitCount(0)
{

}

multi::Barrier::~Barrier()
{
   reset();
}

void multi::Barrier::block()
{
   std::unique_lock<std::mutex> lock(m_mutex);
   ++m_blockedCount;
   if(m_blockedCount < m_maxCount)
   {
      ++m_waitCount;
      m_conditionalBlock.wait(lock, [this]{return m_blockedCount>=m_maxCount;} );
      --m_waitCount;         
   }
   else
   {
      m_conditionalBlock.notify_all();
   }
   // always notify the conditional wait just in case anyone is waiting
   m_conditionalWait.notify_all();
}

void multi::Barrier::reset()
{
   std::unique_lock<std::mutex> lock(m_mutex);
   // force the condition on any waiting threads
   m_blockedCount = m_maxCount;
   if(m_waitCount.load() > 0){
      m_conditionalBlock.notify_all(); 
      // wait until the wait count goes back to zero
      m_conditionalWait.wait(lock, [this]{return m_waitCount.load()<1;});
   }
   // should be safe to reset everything at this point 
   m_blockedCount = 0;
   m_waitCount    = 0;
}

void multi::Barrier::reset(int maxCount)
{
   // all threads should be released after this call
   reset();
   {
      // now safe to update the new max count
      std::unique_lock<std::mutex> lock(m_mutex);
      m_maxCount = maxCount;
   }
}

int multi::Barrier::getMaxCount()const
{ 
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_maxCount; 
}

int multi::Barrier::getBlockedCount()const
{
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_blockedCount; 
}

/**
* Thread is an abstract class.  It provides a 
* general purpose thread interface that handles preliminary setup
* of the std c++11 thread.  It allows one to derive
* from Thread and override the run method.  Your thread should have calls
* to interrupt() whenever your thread is in a location that is
* interruptable.  If cancel is called then any thread that is interruptable will 
* throw an Interrupt and be caught in the base Thread class and then exit 
* the thread.
**/

multi::Thread::Thread()
:m_running(false),
 m_interrupt(false),
 m_pauseBarrier(std::make_shared<multi::Barrier>(1))
{
}

multi::Thread::~Thread()
{
   if(m_thread)
   {
      // we will wait as a sanity but this should be done in derived Thread
      waitForCompletion();
      if(m_thread->joinable()) m_thread->join();
      m_thread = 0;         
   }
   m_running = false;
}

void multi::Thread::start()
{
   if(isInterruptable()||isRunning()) return;
   m_running = true;

   // we are managing the thread internal.  If we are not running then we may need to join
   // before allocating a new thread
   if(m_thread)
   {
      if(m_thread->joinable()) m_thread->join();
   }

   m_thread = std::make_shared<std::thread>(&Thread::runInternal, this);
}

void multi::Thread::setCancel(bool flag)
{
   setInterruptable(flag);
   if(flag)
   {
      // if it was paused we will resume.  Calling resume 
      // will reset the barrier so we can cancel the process
      //
      resume();
   }
}

void multi::Thread::waitForCompletion()
{
   if(m_thread)
   {
      std::unique_lock<std::mutex> lock(m_runningMutex);
      m_runningCondition.wait(lock, [&]{return !isRunning();} );
   }
}

void multi::Thread::pause()
{
   m_pauseBarrier->reset(2);
}

void multi::Thread::resume()
{
   m_pauseBarrier->reset(1);
}

bool multi::Thread::isPaused()const
{
   return (m_pauseBarrier->getBlockedCount()>0);
}

void multi::Thread::sleepInSeconds(unsigned long long seconds)
{
   std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void multi::Thread::sleepInMilliSeconds(unsigned long long millis)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void multi::Thread::sleepInMicroSeconds(unsigned long long micros)
{
   std::this_thread::sleep_for(std::chrono::microseconds(micros));
}

unsigned long long multi::Thread::getNumberOfProcessors()
{
   return std::thread::hardware_concurrency();
}

std::thread::id multi::Thread::getCurrentThreadId()
{
   return std::this_thread::get_id();
}

void multi::Thread::yieldCurrentThread()
{
    std::this_thread::yield();
}

void multi::Thread::interrupt()
{
   if(m_interrupt)
   {
      throw multi::Thread::Interrupt();
   }
   m_pauseBarrier->block();
}

void multi::Thread::setInterruptable(bool flag)
{
   m_interrupt.store(flag, std::memory_order_relaxed);
}

void multi::Thread::runInternal()
{
   try
   {
      if(!isInterruptable())
      {
         run();
      }
   }
   catch(Interrupt& e)
   {
       const char* error = e.what();
   }
   m_running = false;
   m_runningCondition.notify_all();
}

