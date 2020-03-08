#include <multiJobQueue.h>
#include <algorithm> /* for std::find */
#include <iostream>


/**
* This is a very simple block interface.  This allows one to control 
* how their threads are blocked
*
* There is a release state flag that tells the call to block to block the calling
* thread or release the thread(s) that are currently blocked
**/
multi::Block::Block(bool releaseFlag)
:m_release(releaseFlag), m_waitCount(0)
{

}

multi::Block::~Block()
{
   release();
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      if(m_waitCount>0)
      {
         m_conditionalWait.wait(lock, [this]{return m_waitCount.load()<1;});
      }
   }
}

void multi::Block::set(bool releaseFlag)
{
   {
      std::unique_lock<std::mutex> lock(m_mutex);

      m_release = releaseFlag;      
   }
   m_conditionVariable.notify_all();
}

void multi::Block::block()
{
   std::unique_lock<std::mutex> lock(m_mutex);
   if(!m_release)
   {
      ++m_waitCount;
      m_conditionVariable.wait(lock, [this]{
         return (m_release.load() == true);
      });
      --m_waitCount;
      if(m_waitCount < 0) m_waitCount = 0;
   }
   m_conditionVariable.notify_all();   
   m_conditionalWait.notify_all();
}

void multi::Block::block(unsigned long long waitTimeMillis)
{
   std::unique_lock<std::mutex> lock(m_mutex);
   if(!m_release)
   {
      ++m_waitCount;
      m_conditionVariable.wait_for(lock, 
                                   std::chrono::milliseconds(waitTimeMillis),
                                   [this]{
         return (m_release.load() == true);
      });
      --m_waitCount;
      if(m_waitCount < 0) m_waitCount = 0;
   }
   m_conditionVariable.notify_all();   
   m_conditionalWait.notify_all();
}
void multi::Block::release()
{
   {   
      std::unique_lock<std::mutex> lock(m_mutex);
      if(!m_release)
      {
         m_release = true;
      }
      m_conditionVariable.notify_all();
   }
}

void multi::Block::reset()
{
   std::unique_lock<std::mutex> lock(m_mutex);

   m_release   = false;
   m_waitCount = 0;
}



/**
* This is the base implementation for the job queue.  It allows one to add and remove
* jobs and to call the nextJob and, if specified, block the call if no jobs are on
* the queue.  we derived from std::enable_shared_from_this which allows us access to 
* a safe shared 'this' pointer.  This is used internal to our callbacks.
*
* The job queue is thread safe and can be shared by multiple threads.
**/

multiJobQueue::multiJobQueue()
{
}

void multiJobQueue::add(std::shared_ptr<multiJob> job, 
                        bool guaranteeUniqueFlag)
{
   std::shared_ptr<Callback> cb;
   {
      {
         std::lock_guard<std::mutex> lock(m_jobQueueMutex);
         
         if(guaranteeUniqueFlag)
         {
            if(findByPointer(job) != m_jobQueue.end())
            {
               m_block.set(true);
               return;
            }
         }
         cb = m_callback;
      }
      if(cb) cb->adding(getSharedFromThis(), job);
      
      job->ready();
      m_jobQueueMutex.lock();
      m_jobQueue.push_back(job);
      m_jobQueueMutex.unlock();
   }
   if(cb)
   {
      cb->added(getSharedFromThis(), job);
   }
   m_block.set(true);
}

std::shared_ptr<multiJob> multiJobQueue::removeByName(const multiString& name)
{
   std::shared_ptr<multiJob> result;
   std::shared_ptr<Callback> cb;
   if(name.empty()) return result;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      multiJob::List::iterator iter = findByName(name);
      if(iter!=m_jobQueue.end())
      {
         result = *iter;
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
   }      
   m_block.set(!m_jobQueue.empty());
   
   if(cb&&result)
   {
      cb->removed(getSharedFromThis(), result);
   }
   return result;
}
std::shared_ptr<multiJob> multiJobQueue::removeById(const multiString& id)
{
   std::shared_ptr<multiJob> result;
   std::shared_ptr<Callback> cb;
   if(id.empty()) return result;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      multiJob::List::iterator iter = findById(id);
      if(iter!=m_jobQueue.end())
      {
         result = *iter;
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
      m_block.set(!m_jobQueue.empty());
   }
   if(cb&&result)
   {
      cb->removed(getSharedFromThis(), result);
   }
   return result;
}

void multiJobQueue::remove(const std::shared_ptr<multiJob> job)
{
   std::shared_ptr<multiJob> removedJob;
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      multiJob::List::iterator iter = std::find(m_jobQueue.begin(), m_jobQueue.end(), job);
      if(iter!=m_jobQueue.end())
      {
         removedJob = (*iter);
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
   }
   if(cb&&removedJob)
   {
      cb->removed(getSharedFromThis(), removedJob);
   }
}

void multiJobQueue::removeStoppedJobs()
{
   multiJob::List removedJobs;
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      cb = m_callback;
      multiJob::List::iterator iter = m_jobQueue.begin();
      while(iter!=m_jobQueue.end())
      {
         if((*iter)->isStopped())
         {
            removedJobs.push_back(*iter);
            iter = m_jobQueue.erase(iter);
         }
         else 
         {
            ++iter;
         }
      }
   }
   if(!removedJobs.empty())
   {
      if(cb)
      {
         multiJob::List::iterator iter = removedJobs.begin();
         while(iter!=removedJobs.end())
         {
            cb->removed(getSharedFromThis(), (*iter));
            ++iter;
         }
      }
      removedJobs.clear();
   }
}

void multiJobQueue::clear()
{
   multiJob::List removedJobs(m_jobQueue);
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      m_jobQueue.clear();
      cb = m_callback;
   }
   if(cb)
   {
      for(multiJob::List::iterator iter=removedJobs.begin();iter!=removedJobs.end();++iter)
      {
         cb->removed(getSharedFromThis(), (*iter));
      }
   }
}

std::shared_ptr<multiJob> multiJobQueue::nextJob(bool blockIfEmptyFlag)
{
   m_jobQueueMutex.lock();
   bool emptyFlag = m_jobQueue.empty();
   m_jobQueueMutex.unlock();
   if (blockIfEmptyFlag && emptyFlag)
   {
      m_block.block();
   }
   
   std::shared_ptr<multiJob> result;
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   
   if (m_jobQueue.empty())
   {
      m_block.set(false);
      return result;
   }
   
   multiJob::List::iterator iter= m_jobQueue.begin();
   while((iter != m_jobQueue.end())&&
         (((*iter)->isCanceled())))
   {
      (*iter)->finished(); // mark the ob as being finished 
      iter = m_jobQueue.erase(iter);
   }
   if(iter != m_jobQueue.end())
   {
      result = *iter;
      m_jobQueue.erase(iter);
   }
   m_block.set(!m_jobQueue.empty());

   return result;
}
void multiJobQueue::releaseBlock()
{
   m_block.release();
}
bool multiJobQueue::isEmpty()const
{
   // std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   // return m_jobQueue.empty();
   m_jobQueueMutex.lock();
   bool result =  m_jobQueue.empty();
   m_jobQueueMutex.unlock();
   return result;
}

unsigned int multiJobQueue::size()
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   return (unsigned int) m_jobQueue.size();
}

multiJob::List::iterator multiJobQueue::findById(const multiString& id)
{
   if(id.empty()) return m_jobQueue.end();
   multiJob::List::iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(id == (*iter)->id())
      {
         return iter;
      }
      ++iter;
   }  
   return m_jobQueue.end();
}

multiJob::List::iterator multiJobQueue::findByName(const multiString& name)
{
   if(name.empty()) return m_jobQueue.end();
   multiJob::List::iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(name == (*iter)->name())
      {
         return iter;
      }
      ++iter;
   }  
   return m_jobQueue.end();
}

multiJob::List::iterator multiJobQueue::findByPointer(const std::shared_ptr<multiJob> job)
{
   return std::find(m_jobQueue.begin(),
                    m_jobQueue.end(),
                    job);
}

multiJob::List::iterator multiJobQueue::findByNameOrPointer(const std::shared_ptr<multiJob> job)
{
   multiString n = job->name();
   multiJob::List::iterator iter = std::find_if(m_jobQueue.begin(), m_jobQueue.end(), [n, job](const std::shared_ptr<multiJob> jobIter){
      bool result = (jobIter == job);
      if(!result&&!n.empty()) result = jobIter->name() == n;
      return result;
   });
   // multiJob::List::iterator iter = m_jobQueue.begin();
   // while(iter != m_jobQueue.end())
   // {
   //    if((*iter) == job)
   //    {
   //       return iter;
   //    }
   //    else if((!n.empty())&&
   //            (job->name() == (*iter)->name()))
   //    {
   //       return iter;
   //    }
   //    ++iter;
//   }  
   
   return iter;
}

bool multiJobQueue::hasJob(std::shared_ptr<multiJob> job)
{
   multiJob::List::const_iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(job == (*iter))
      {
         return true;
      }
      ++iter;
   }
   
   return false;
}

void multiJobQueue::setCallback(std::shared_ptr<Callback> c)
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   m_callback = c;
}

std::shared_ptr<multiJobQueue::Callback> multiJobQueue::callback()
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   return m_callback;
}
