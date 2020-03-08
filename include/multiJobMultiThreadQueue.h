#ifndef multiJobMultiThreadQueue_HEADER
#define multiJobMultiThreadQueue_HEADER
#include <multiJobThreadQueue.h>
#include <mutex>
#include <vector>

/**
* This allocates a thread pool used to listen on a shared job queue
*
* @code
* #include <Thread.h>
* #include <multiJob.h>
* #include <multiJobMultiThreadQueue.h>
* #include <multiJobQueue.h>
* #include <memory>
* #include <iostream>
* 
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
* 
* int main(int argc, char *argv[])
* {
*    int nThreads = 5;
*    int nJobs = 10;
*    std::shared_ptr<multiJobQueue> jobQueue = std::make_shared<multiJobQueue>();
*    std::shared_ptr<multiJobMultiThreadQueue> jobThreadQueue = std::make_shared<multiJobMultiThreadQueue>(jobQueue, nThreads);
*    for(int i = 0; i < nJobs; ++i)
*    {
*       std::shared_ptr<TestJob> job = std::make_shared<TestJob>();
*       job->setCallback(std::make_shared<MyCallback>());
*       jobQueue->add(job);
*    }
* 
*    while(jobThreadQueue->hasJobsToProcess())
*    {
*       multi::Thread::sleepInMilliSeconds(10);
*    }
* 
*    std::cout << "Finished and cancelling thread queue\n";
*    jobThreadQueue->cancel();
*    jobThreadQueue->waitForCompletion();
* 
*    return 0;
* }
* @endcode
*/
class OSSIM_DLL multiJobMultiThreadQueue
{
public:
   typedef std::vector<std::shared_ptr<multiJobThreadQueue> > ThreadQueueList;
   
   /**
   * allows one to create a pool of threads with a shared job queue
   */
   multiJobMultiThreadQueue(std::shared_ptr<multiJobQueue> q=0, 
                            unsigned int nThreads=0);
   /**
   * Will cancel all threads and wait for completion and clear the thread queue
   * list
   */
   virtual ~multiJobMultiThreadQueue();

   /**
   * @return the job queue
   */
   std::shared_ptr<multiJobQueue> getJobQueue();

   /**
   * @return the job queue
   */
   const std::shared_ptr<multiJobQueue> getJobQueue()const;

   /**
   * set the job queue to all threads
   *
   * @param q the job queue to set
   */
   void setJobQueue(std::shared_ptr<multiJobQueue> q);

   /**
   * Will set the number of threads
   */
   void setNumberOfThreads(unsigned int nThreads);

   /**
   * @return the number of threads
   */
   unsigned int getNumberOfThreads() const;

   /**
   * @return the number of threads that are busy
   */
   unsigned int numberOfBusyThreads()const;

   /**
   * @return true if all threads are busy and false otherwise
   */
   bool areAllThreadsBusy()const;

   /**
   * @return true if it has jobs that it's processing
   */
   bool hasJobsToProcess()const;

   /**
   * Allows one to cancel all threads
   */
   void cancel();

   /**
   * Allows on to wait for all thread completions.  Usually called after
   * @see cancel
   */
   void waitForCompletion();

protected:
   mutable std::mutex             m_mutex;
   std::shared_ptr<multiJobQueue> m_jobQueue;
   ThreadQueueList                m_threadQueueList;
};

#endif
