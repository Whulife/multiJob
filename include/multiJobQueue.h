//**************************************************************************************************
//                          OSSIM -- Open Source Software Image Map
//
// LICENSE: See top level LICENSE.txt file.
//
//**************************************************************************************************
//  $Id$
#ifndef multiJobQueue_HEADER
#define multiJobQueue_HEADER

#include <multiJob.h>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <atomic>
namespace multi{

   /**
   * This is a very simple block interface.  This allows one to control 
   * how their threads are blocked
   *
   * There is a release state flag that tells the call to block to block the calling
   * thread or release the thread(s) that are currently blocked
   *
   * For a very simple use case we will start a thread and call block and have
   * the main sleep for 2 seconds before releasing the thread
   * @code
   * #include <Block.h>
   * #include <Thread.h>
   * std::shared_ptr<multi::Block> block = std::make_shared<multi::Block>();
   * class TestThread : public multi::Thread
   * {
   * public:
   * 
   * protected:
   *    virtual void run(){
   *      block->block();
   *       std::cout << "STARING!!!!!!!\n";
   *    }
   * };
   * int main(int argc, char *argv[])
   * {
   *    TestThread t1;
   *    t1.start();
   * 
   *    std::cout << "WAITING 2 SECOND to release block\n";
   *    multi::Thread::sleepInSeconds(2);
   *    block->release();
   *    multi::Thread::sleepInSeconds(2);
   * }
   * @endcode
   */
   class OSSIM_DLL Block
   {
   public:
      /**
      * Allows one the construct a Block with a release state.
      */
      Block(bool releaseFlag=false);

      /**
      * Destructor
      *
      * Will set internally call release
      */
      ~Block();

      /**
      * Will set the relase flag and wake up all threads to test the condition again.
      */
      void set(bool releaseFlag);

      /**
      * Will block the calling thread based on the internal condition.  If the internal
      * condition is set to release then it will return without blocking.
      */
      void block();

      /**
      * Will block the calling thread base on the internal condition.  If the internal 
      * condition is set to release the it will return without blocking.  If the internal
      * condition is set to not release then it will block for the specified time in 
      * milliseconds
      *
      * @param waitTimeMillis specifies the amount of time to wait for the release
      */
      void block(unsigned long long waitTimeMillis);

      /**
      * Releases the threads and will not return until all threads are released
      */
      void release();

      /**
      * Simple reset the values.  Will not do any releasing
      */
      void reset();

   private:
      /**
      * Used by the conditions
      */
      mutable std::mutex      m_mutex;

      /**
      * The release state.
      */
      std::atomic<bool>       m_release;

      /**
      * Condition that tests the release state
      */
      std::condition_variable m_conditionVariable;

      /**
      * Used to count the number of threads blocked or
      * waiting on the condition
      */
      std::atomic<int> m_waitCount;

      /**
      * Will be used for destructing and releasing.
      * resetting should only happen in the main 
      * thread
      */ 
      std::condition_variable m_conditionalWait;
   };
}

/**
* This is the base implementation for the job queue.  It allows one to add and remove
* jobs and to call the nextJob and, if specified, block the call if no jobs are on
* the queue.  we derived from std::enable_shared_from_this which allows us access to 
* a safe shared 'this' pointer.  This is used internal to our callbacks.
*
* The job queue is thread safe and can be shared by multiple threads.
*
* Here is a quick code example on how to create a shared queue and to attach
* a thread to it.  In this example we do not block the calling thread for nextJob
* @code
* #include <multi/base/Thread.h>
* #include <multi/parallel/multiJob.h>
* #include <multi/parallel/multiJobQueue.h>
* #include <iostream>
* class TestJobQueueThread : public multi::Thread
* {
* public:
*    TestJobQueueThread(std::shared_ptr<multiJobQueue> q):m_q(q){}
*    void run()
*    {
*       if(m_q)
*       {
*          while(true)
*          {
*             interrupt();
*             std::shared_ptr<multiJob> job = m_q->nextJob(false);
*             if(job)
*             {
*                job->start();
*             }
*             yieldCurrentThread();
*             sleepInMilliSeconds(20);
*          }
*       }
*    }
* 
* private:
*   std::shared_ptr<multiJobQueue> m_q;
* };
* 
* class TestJob : public multiJob
* {
* public:
*    virtual void run()
*    {
*       std:cout << "Running Job\n";
*       multi::Thread::sleepInSeconds(2);
*       std::cout << "Finished Running Job\n";
*    }
* };
* int main(int argc, char *argv[])
* {
*    std::shared_ptr<multiJobQueue> q = std::make_shared<multiJobQueue>();
*    std::shared_ptr<TestJobQueueThread> jobQueueThread = std::make_shared<TestJobQueueThread>(q);
* 
*    jobQueueThread->start();
* 
*    q->add(std::make_shared<TestJob>());
*    q->add(std::make_shared<TestJob>());
*    q->add(std::make_shared<TestJob>());
* 
*    multi::Thread::sleepInSeconds(10);
*    jobQueueThread->cancel();
*    jobQueueThread->waitForCompletion();
* }
* @endcode
*/
class OSSIM_DLL multiJobQueue : public std::enable_shared_from_this<multiJobQueue>
{
public:
   /**
   * The callback allows one to attach and listen for certain things.  In 
   * the multiJobQueue it will notify just before adding a job, after adding a job
   * and if a job is removed.
   */
   class OSSIM_DLL Callback
   {
   public:
      Callback(){}

      /**
      * Called just before a job is added
      * 
      * @param q Is a shared_ptr to 'this' job queue
      * @param job Is a shared ptr to the job we are adding
      */
      virtual void adding(std::shared_ptr<multiJobQueue> /*q*/, 
                          std::shared_ptr<multiJob> /*job*/){};

      /**
      * Called after a job is added to the queue
      * @param q Is a shared_ptr to 'this' job queue
      * @param job Is a shared ptr to the job we are added     
      */
      virtual void added(std::shared_ptr<multiJobQueue> /*q*/, 
                         std::shared_ptr<multiJob> /*job*/){}


      /**
      * Called after a job is removed from the queue
      * @param q Is a shared_ptr to 'this' job queue
      * @param job Is a shared ptr to the job we have removed
      */
      virtual void removed(std::shared_ptr<multiJobQueue> /*q*/, 
                           std::shared_ptr<multiJob>/*job*/){}
   };

   /**
   * Default constructor
   */
   multiJobQueue();
  
   /**
   * This is the safe way to create a std::shared_ptr for 'this'.  Calls the derived
   * method shared_from_this
   */  
   std::shared_ptr<multiJobQueue> getSharedFromThis(){
      return shared_from_this();
   }

   /**
   * This is the safe way to create a std::shared_ptr for 'this'.  Calls the derived
   * method shared_from_this
   */  
   std::shared_ptr<const multiJobQueue> getSharedFromThis()const{
      return shared_from_this();
   }

   /**
   * Will add a job to the queue and if the guaranteeUniqueFlag is set it will
   * scan and make sure the job is not on the queue before adding
   *
   * @param job The job to add to the queue.
   * @param guaranteeUniqueFlag if set to true will force a find to make sure the job
   *        does not already exist
   */
   virtual void add(std::shared_ptr<multiJob> job, bool guaranteeUniqueFlag=true);
   
   /**
   * Allows one to remove a job passing in it's name.
   *
   * @param name The job name
   * @return a shared_ptr to the job.  This will be nullptr if not found.
   */
   virtual std::shared_ptr<multiJob> removeByName(const multiString& name);

   /**
   * Allows one to remove a job passing in it's id.
   *
   * @param id The job id
   * @return a shared_ptr to the job.  This will be nullptr if not found.
   */
   virtual std::shared_ptr<multiJob> removeById(const multiString& id);

   /**
   * Allows one to pass in a job pointer to remove
   *
   * @param job the job you wish to remove from the list
   */
   virtual void remove(const std::shared_ptr<multiJob> Job);

   /**
   * Will remove any stopped jobs from the queue
   */
   virtual void removeStoppedJobs();

   /**
   * Will clear the queue
   */
   virtual void clear();

   /**
   * Will grab the next job on the list and will return the job or 
   * a null shared_ptr.  You can block the caller if the queueis empty forcing it
   * to wait for more jobs to come onto the queue
   *
   * @param blockIfEmptyFlag If true it will block the calling thread until more jobs appear
   *        on the queue.  If false, it will return without blocking
   * @return a shared pointer to a job
   */
   virtual std::shared_ptr<multiJob> nextJob(bool blockIfEmptyFlag=true);

   /**
   * will release the block and have any blocked threads continue
   */
   virtual void releaseBlock();

   /**
   * @return true if the queue is empty false otherwise
   */
   bool isEmpty()const;

   /**
   * @return the number of jobs on the queue
   */
   unsigned int size();

   /**
   *  Allows one to set the callback to the list
   *
   * @param c shared_ptr to a callback
   */
   void setCallback(std::shared_ptr<Callback> c);

   /**
   * @return the callback
   */
   std::shared_ptr<Callback> callback();
   
protected:
   /**
   * Internal method that returns an iterator
   *
   * @param the id of the job to search for
   * @return the iterator
   */
   multiJob::List::iterator findById(const multiString& id);

   /**
   * Internal method that returns an iterator
   *
   * @param name the name of the job to search for
   * @return the iterator
   */
   multiJob::List::iterator findByName(const multiString& name);

   /**
   * Internal method that returns an iterator
   *
   * @param job the job to search for
   * @return the iterator
   */
   multiJob::List::iterator findByPointer(const std::shared_ptr<multiJob> job);

   /**
   * Internal method that returns an iterator
   * 
   * @param job it will find by the name or by the pointer
   * @return the iterator
   */
   multiJob::List::iterator findByNameOrPointer(const std::shared_ptr<multiJob> job);

   /**
   * Internal method that determines if we have the job
   * 
   * @param job the job you wish to search for
   */
   bool hasJob(std::shared_ptr<multiJob> job);
   
   mutable std::mutex m_jobQueueMutex;
   multi::Block m_block;
   multiJob::List m_jobQueue;
   std::shared_ptr<Callback> m_callback;
};

#endif
