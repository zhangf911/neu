/*

      ___           ___           ___
     /\__\         /\  \         /\__\
    /::|  |       /::\  \       /:/  /
   /:|:|  |      /:/\:\  \     /:/  /
  /:/|:|  |__   /::\~\:\  \   /:/  /  ___
 /:/ |:| /\__\ /:/\:\ \:\__\ /:/__/  /\__\
 \/__|:|/:/  / \:\~\:\ \/__/ \:\  \ /:/  /
     |:/:/  /   \:\ \:\__\    \:\  /:/  /
     |::/  /     \:\ \/__/     \:\/:/  /
     /:/  /       \:\__\        \::/  /
     \/__/         \/__/         \/__/


The Neu Framework, Copyright (c) 2013-2014, Andrometa LLC
All rights reserved.

neu@andrometa.net
http://neu.andrometa.net

Neu can be used freely for commercial purposes. If you find Neu
useful, please consider helping to support our work and the evolution
of Neu by making a PayPal donation to: neu@andrometa.net

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
 
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
 
3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
*/

#include <neu/NProc.h>

#include <iostream>
#include <queue>

#include <neu/NVSemaphore.h>
#include <neu/NThread.h>
#include <neu/NBasicMutex.h>
#include <neu/NRWMutex.h>

using namespace std;
using namespace neu;

namespace{
  
  class State{
  public:
    NProc* np;
    
    State(NProc* np)
    : np(np),
    queueCount_(0),
    terminated_(false){}
    
    void queued(){
      ++queueCount_;
    }
    
    bool dequeued(){
      return --queueCount_ == 0 && terminated_;
    }
    
    bool terminate(){
      terminated_ = true;
      return queueCount_ == 0;
    }
    
    bool terminated(){
      return terminated_;
    }
    
  private:
    atomic<uint32_t> queueCount_;
    atomic_bool terminated_;
  };
  
} // end namespace

namespace neu{
  
  class NProcTask_{
  public:
    
    class Item{
    public:
      Item(State* s, nvar& r, double p)
      : s(s),
      r(move(r)),
      p(p){}
      
      State* s;
      nvar r;
      double p;
    };
    
    class Queue{
    public:
      Queue(){}
      
      ~Queue(){}
      
      void dealloc(NProcTask* task){
        Item* item;
        State* s;
        
        while(!queue_.empty()){
          item = queue_.top();
          queue_.pop();
          
          task->dealloc(item->r);
          
          s = item->s;
          
          if(s->dequeued()){
            delete s->np;
          }
          
          delete item;
        }
      }
      
      void put(Item* item){
        mutex_.lock();
        queue_.push(item);
        mutex_.unlock();
        
        sem_.release();
      }
      
      Item* get(){
        if(!sem_.acquire()){
          return 0;
        }

        mutex_.lock();
        Item* item = queue_.top();
        queue_.pop();
        mutex_.unlock();
        return item;
      }

      void enable(){
        sem_.enable();
      }
      
      void disable(){
        sem_.disable();
      }
      
    private:
      struct Compare_{
        bool operator()(const Item* i1, const Item* i2) const{
          return i1->p < i2->p;
        }
      };
      
      typedef priority_queue<Item*, vector<Item*>, Compare_> Queue_;
      
      Queue_ queue_;
      NVSemaphore sem_;
      NBasicMutex mutex_;
    };
    
    class Thread : public NThread{
    public:
      Thread(Queue& queue)
      : queue_(queue){}
      
      void run(){
        State* s;
        Item* item;
        
        for(;;){
          item = queue_.get();
          
          if(item){
            s = item->s;
            
            s->np->run(item->r);
            
            if(s->dequeued()){
              delete s->np;
            }
          
            delete item;
          }
          else{
            break;
          }
        }
      }
      
    private:
      Queue& queue_;
    };
    
    NProcTask_(NProcTask* o, size_t threads)
    : o_(o),
    active_(true){
      
      for(size_t i = 0; i < threads; ++i){
        Thread* thread = new Thread(q_);
        threadVec_.push_back(thread);
        thread->start();
      }
    }
    
    ~NProcTask_(){
      shutdown();
    }
    
    void shutdown(){
      if(!active_){
        return;
      }
      
      active_ = false;
      
      q_.disable();
      
      for(Thread* t : threadVec_){
        t->join();
        delete t;
      }
      
      q_.dealloc(o_);
      
      for(auto& itr : stateMap_){
        delete itr.second;
      }
    }
    
    void queue(NProc* proc, nvar& r, double priority){
      State* s = getState(proc);
      
      if(s->terminated()){
        return;
      }
      
      s->queued();
      Item* item = new Item(s, r, priority);
      q_.put(item);
    }
    
    State* getState(NProc* np){
      mutex_.readLock();

      auto itr = stateMap_.find(np);
      if(itr == stateMap_.end()){
        mutex_.unlock();
      
        State* state = new State(np);

        mutex_.writeLock();
        stateMap_.insert({np, state});
        mutex_.unlock();
        
        return state;
      }
      
      State* s = itr->second;
      mutex_.unlock();
      
      return s;
    }
    
    bool terminate(NProc* np){
      mutex_.readLock();
      
      auto itr = stateMap_.find(np);
      if(itr == stateMap_.end()){
        mutex_.unlock();
        return true;
      }
      
      State* s = itr->second;
      mutex_.unlock();
      
      return s->terminate();
    }
    
  private:
    typedef NVector<Thread*> ThreadVec_;
    typedef NHashMap<NProc*, State*> StateMap_;
    
    NProcTask* o_;
    Queue q_;
    ThreadVec_ threadVec_;
    bool active_;
    StateMap_ stateMap_;
    NRWMutex mutex_;
  };
  
} // end namespace neu


NProcTask::NProcTask(size_t threads){
  x_ = new NProcTask_(this, threads);
}

NProcTask::~NProcTask(){
  delete x_;
}

void NProcTask::shutdown(){
  x_->shutdown();
}

void NProcTask::queue(NProc* proc, nvar& r, double priority){
  x_->queue(proc, r, priority);
}

bool NProcTask::terminate(NProc* proc){
  return x_->terminate(proc);
}
