#include <iostream>
#include <vector>
#include <Thread.h>
int nThreads = 2;
multi::Barrier barrierStart(nThreads);
// one more for main thread
multi::Barrier barrierFinished(nThreads+1);

class TestThread : public multi::Thread
{
public:
 TestThread():multi::Thread(){}
 ~TestThread(){
    waitForCompletion();
 }

protected:
 virtual void run()
 {
    barrierStart.block();
    std::cout << "THREAD: " << getCurrentThreadId() << "\n";
    for(int x =0 ; x < 10;++x){
       sleepInMilliSeconds(100);
       interrupt();
    }
    barrierFinished.block();
 }
};

int main(int argc, char* argv[])
{
        std::vector<std::shared_ptr<TestThread> > threads(nThreads);
        for(auto& thread:threads)
        {
         thread = std::make_shared<TestThread>();
         thread->start();
        }
        // block main until barrier enters their finished state
        barrierFinished.block();

        std::cout << "Redo:\n";
        // you can also reset the barriers and run again
        barrierFinished.reset();
        barrierStart.reset();
        for(auto& thread:threads)
        {
         thread->start();
        }
        barrierFinished.block();
}

