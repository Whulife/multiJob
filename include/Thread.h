#ifndef multiThread_HEADER
#define multiThread_HEADER 1
#include <multiConstants.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

namespace multi{

   /**
   * Barrier is a class used to block threads so we can synchronize and entry point.
   *
   * In this example we show how to block the threads so they all start at the 
   * same time when executing their work.
   * Example:
   *
   * @code
   * #include <multi/base/Thread.h>
   * #include <multi/base/Barrier.h>
   * int nThreads = 2;
   * multi::Barrier barrierStart(nThreads);
   * // one more for main thread
   * multi::Barrier barrierFinished(nThreads+1);
   *
   * class TestThread : public multi::Thread
   *  {
   *  public:
   *      TestThread():multi::Thread(){}
   *      ~TestThread(){
   *         waitForCompletion();
   *      }
   *
   *   protected:
   *      virtual void run()
   *      {
   *         barrierStart.block();
   *         for(int x =0 ; x < 10;++x){
   *            std::cout << "THREAD: " << getCurrentThreadId() << "\n";
   *            sleepInMilliSeconds(10);
   *            interrupt();
   *         }
   *         barrierFinished.block();
   *      }
   *   };
   *
   * int main(int agrc, char* argv[])
   * {
   *    std::vector<std::shared_ptr<TestThread> > threads(nThreads);
   *    for(auto& thread:threads)
   *    {
   *      thread = std::make_shared<TestThread>();
   *      thread->start();
   *    }
   *    // block main until barrier enters their finished state
   *    barrierFinished.block();
   *
   *    // you can also reset the barriers and run again
   *    barrierFinished.reset();
   *    barrierStart.reset();
   *    for(auto& thread:threads)
   *    {
   *      thread->start();
   *    }
   *    barrierFinished.block();
   * }
   *
   * @endcode
   */
   class OSSIM_DLL Barrier 
   {
   public:
      /**
      * Constructor
      * 
      * @param n is the number of threads you wish to block
      */
      Barrier(int n);

      /**
      * Destructor will reset and release all blocked threads.
      */
      ~Barrier();

      /**
      * block will block the thread based on a wait condition.  it will verify 
      * if the thread can be blocked by testing if the number
      * of blocked threads is less than the total number to blocked threads.  If 
      * the total is reached then all threads are notified and woken up and released  
      */
      void block();

      /**
      * Will reset the barrier to the original values.  
      * It will also release all blocked threads and wait for their release
      * before resetting.
      */
      void reset();

      /**
      * Will reset the barrier to a new block count.  
      * It will also release all blocked threads and wait for their release
      * before resetting.
      *
      * @param maxCount is the max number of threads to block
      */
      void reset(int maxCount);

      int getMaxCount()const;
      int getBlockedCount()const;
   protected:
      int              m_maxCount;
      int              m_blockedCount;
      std::atomic<int> m_waitCount;
      mutable std::mutex       m_mutex;
      std::condition_variable  m_conditionalBlock;

      /**
      * Will be used for destructing and resetting.
      * resetting should only happen in the main 
      * thread
      */ 
      std::condition_variable m_conditionalWait;
   };
}


namespace multi{

   /**
   * Thread is an abstract class.  It provides a 
   * general purpose thread interface that handles preliminary setup
   * of the std c++11 thread.  It allows one to derive
   * from Thread and override the run method.  Your thread should have calls
   * to interrupt() whenever your thread is in a location that is
   * interruptable.  If cancel is called then any thread that is interruptable will 
   * throw an Interrupt and be caught in the base Thread class and then exit 
   * the thread.
   *
   * Example:
   * @code
   * #include <Thread.h>
   * #include <Barrier.h>
   * class TestThread : public multi::Thread
   *  {
   *  public:
   *      TestThread():multi::Thread(){}
   *      ~TestThread(){
   *         waitForCompletion();
   *      }
   *
   *   protected:
   *      virtual void run()
   *      {
   *         barrierStart.block();
   *         for(int x =0 ; x < 10;++x){
   *            std::cout << "THREAD: " << getCurrentThreadId() << "\n";
   *            // simulate 10 milliseconds of uninterruptable work
   *            sleepInMilliSeconds(10);
   *            interrupt();
   *         }
   *         barrierFinished.block();
   *      }
   *   };
   * int main(int agrc, char* argv[])
   * {
   *    std::vector<std::shared_ptr<TestThread> > threads(nThreads);
   *    for(auto& thread:threads)
   *    {
   *      thread = std::make_shared<TestThread>();
   *      thread->start();
   *    }
   *
   *    // now let's wait for each thread to finish 
   *    // before exiting
   *    for(auto& thread:threads)
   *    {
   *      thread->waitForCompletion();
   *    }
   * }
   * @endcode
   */ 
   class OSSIM_DLL Thread
   {
   public:
      /**
      * This is an Interrupt exception that is thrown if the @see cancel()
      * is called and a call to @see interrupt() is made.
      */
      class Interrupt : public std::exception{
      public:
         Interrupt(const std::string& what=""):m_what(what){}
         virtual const char* what() const throw(){return m_what.c_str();}
      protected:
         std::string m_what;
      };
      /**
      * Constructor for this thread
      */
      Thread();

      /**
      * Destructor for this thread.  It will determine if this thread is joinable
      * to the main thread and if so it will do a join before continuing.  If
      * this is not done then an exeption is thrown by the std.
      */
      virtual ~Thread();

      /**
      * Will actually start the thread and will call the @see internalRun.
      */
      void start();

      /**
      * @return true if the current thread is running and false otherwise.
      */
      bool isRunning()const{return m_running.load(std::memory_order_relaxed);}

      /**
      * This is typically set if @see cancel() is called or if @see setCancel
      * is called with argument set to true.
      *
      * @return true if the thread is interruptable and false otherwise.
      */
      bool isInterruptable()const{return m_interrupt.load(std::memory_order_relaxed);}

      /**
      * This basically requests that the thread be canceled.  @see setCancel.  Note,
      * cancellation is not done immediately and a thread is only canceled if derived
      * classes call the interrupt().
      *
      * we will make these virtual just in case derived classes want to set conditions
      */
      virtual void cancel(){setCancel(true);}

      /**
      * @param flag if true will enable the thread to be interruptable and if false
      *        the thread is not interruptable.
      */
      virtual void setCancel(bool flag);

      /**
      * Convenience to allow one to wait for this thread to finish it's work.
      *
      * Allow this to be overriden.
      */
      virtual void waitForCompletion();

      /**
      * Enables the thread to be paused.  If the interrupt is called
      * it will block the thread
      */
      void pause();

      /**
      * This will resume a blocked thread.
      */
      void resume();

      /**
      * @return true if the thread is blocked and false otherwise
      */
      bool isPaused()const;

      /**
      * Utility method to allow one to sleep in seconds
      *
      * @param seconds to sleep
      */
      static void sleepInSeconds(unsigned long long seconds);

      /**
      * Utility method to allow one to sleep in milliseconds
      *
      * @param millis to sleep
      */
      static void sleepInMilliSeconds(unsigned long long millis);

      /**
      * Utility method to allow one to sleep in microseconds
      *
      * @param micros to sleep
      */
      static void sleepInMicroSeconds(unsigned long long micros);

      /**
      * Utility method to get the current thread ID
      *
      * @return current thread ID
      */
      static std::thread::id getCurrentThreadId();

      /**
      * Utility to return the number of processors  (concurrent threads)
      */
      static unsigned long long getNumberOfProcessors();
      /**
      * Will yield the current thread.
      */
      static void yieldCurrentThread();

   protected:
      /**
      * This method must be overriden and is the main entry
      * point for any work that needs to be done
      */
      virtual void run()=0;

      /**
      * This is the interrupt interface and will cause an internal exception that
      * is caught by @see runInternal
      */
      virtual void interrupt();

      /**
      * runInternal sets up internal flags such as setting m_running to true and checks
      * to make sure it's not interrupted and will then call the @see run() method.
      *
      * runInternal also will trap any Interrupt exceptions.  If the thread is interruptable
      * and the work calls interrupt then an exception is thrown and the work is stopped and 
      * the execution of the thread is marked as not running and returns.
      */
      virtual void runInternal();

   private:
      std::shared_ptr<std::thread>    m_thread;
      std::atomic<bool>               m_running;
      std::atomic<bool>               m_interrupt;
      std::shared_ptr<multi::Barrier> m_pauseBarrier;
      std::condition_variable         m_runningCondition;
      mutable std::mutex              m_runningMutex;

      /**
      * @see cancel and @see setCancel
      */
      void setInterruptable(bool flag);
   };
}

#endif
