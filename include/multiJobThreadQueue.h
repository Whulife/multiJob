#ifndef multiJobThreadQueue_HEADER
#define multiJobThreadQueue_HEADER
#include <multiJobQueue.h>
#include <Thread.h>
#include <mutex>

/**
* multiJobThreadQueue allows one to instantiate a thread with a shared
* queue. the thread will block if the queue is empty and will continue
* to pop jobs off the queue calling the start method on the job.  Once it
* finishes the job it is disguarded and then the next job will be popped off the 
* queue.
*  
* @code
* class TestJob : public multiJob
* {
* public:
*    TestJob(){}
* protected:
*    virtual void run()
*    {
*       multi::Thread::sleepInSeconds(2);
*    }
* };
* class MyCallback : public multiJobCallback
* {
* public:
*    MyCallback(){}
*    virtual void started(std::shared_ptr<multiJob> job)  
*    {
*       std::cout << "Started job\n";
*       multiJobCallback::started(job);
*    }
*    virtual void finished(std::shared_ptr<multiJob> job) 
*    {
*       std::cout << "Finished job\n";
*       multiJobCallback::finished(job);
*    }
* };
* int main(int argc, char *argv[])
* {
*    std::shared_ptr<multiJobQueue> jobQueue = std::make_shared<multiJobQueue>();
*    std::shared_ptr<multiJobThreadQueue> jobThreadQueue = std::make_shared<multiJobThreadQueue>(jobQueue);
*    jobThreadQueue->start();
*    std::shared_ptr<TestJob> job = std::make_shared<TestJob>();
*    job->setCallback(std::make_shared<MyCallback>());
*    jobQueue->add(job);
*    std::cout << "Waiting 5 seconds before terminating\n";
*    multi::Thread::sleepInSeconds(5);
*    jobThreadQueue->cancel();
*    jobThreadQueue->waitForCompletion();
* 
*    return 0;
* }
* @endcode
*/
class OSSIM_DLL multiJobThreadQueue : public multi::Thread
{
public:
   /**
   * constructor that allows one to instantiat the thread with 
   * a shared job queue.
   *
   * @param jqueue shared job queue
   */
   multiJobThreadQueue(std::shared_ptr<multiJobQueue> jqueue=0);
   
   /**
   * destructor.  Will terminate the thread and stop current jobs
   */
   virtual ~multiJobThreadQueue();

   /**
   *
   * Sets the shared queue that this thread will be pulling jobs from
   *
   * @param jqueue the shared job queue to set
   */
   void setJobQueue(std::shared_ptr<multiJobQueue> jqueue);
   
   /**
   * @return the current shared job queue
   */
   std::shared_ptr<multiJobQueue> getJobQueue();
   
   /**
   * @return the current shared job queue
   */
   const std::shared_ptr<multiJobQueue> getJobQueue() const; 
   
   /**
   * @return the current job that is being handled.
   */
   std::shared_ptr<multiJob> currentJob();
   
   /**
   * Will cancel the current job
   */
   void cancelCurrentJob();

   /**
   * @return is the queue valid
   */
   bool isValidQueue()const;
   
   /**
   * This is method is overriden from the base thread class and is
   * the main entry point of the thread
   */
   virtual void run();
   
   /**
   * Sets the done flag.
   *
   * @param done the value to set
   */
   void setDone(bool done);
   
   /**
   * @return if the done flag is set
   */
   bool isDone()const;

   /**
   * Cancels the thread
   */
   virtual void cancel();

   /**
   * @return true if the queue is empty
   *         false otherwise.
   */
   bool isEmpty()const;
   
   /**
   * @return true if a job is currently being processed
   *         false otherwise.
   */
   bool isProcessingJob()const;
   
   /**
   * @return true if there are still jobs to be processed
   *         false otherwise.
   */
   bool hasJobsToProcess()const;
   
protected:
   /**
   * Internal method.  If setJobQueue is set on this thread
   * it will auto start this thread.
   */
   void startThreadForQueue();

   /**
   * Will return the next job on the queue
   */
   virtual std::shared_ptr<multiJob> nextJob();
   
   bool                           m_doneFlag;
   mutable std::mutex             m_threadMutex;
   std::shared_ptr<multiJobQueue> m_jobQueue;
   std::shared_ptr<multiJob>      m_currentJob;
   
};

#endif
