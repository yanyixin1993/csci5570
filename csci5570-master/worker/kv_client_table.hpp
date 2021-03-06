#pragma once

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/third_party/sarray.h"
#include "base/threadsafe_queue.hpp"
#include "worker/abstract_callback_runner.hpp"

#include <cinttypes>
#include <vector>

namespace csci5570 {

/**
 * Provides the API to users, and implements the worker-side abstraction of model
 * Each model in one application is uniquely handled by one KVClientTable
 *
 * @param Val type of model parameter values
 */
template <typename Val>
class KVClientTable {
 using KVPairs = std::pair<third_party::SArray<Key>, third_party::SArray<double>>;
 public:
  /**
   * @param app_thread_id       user thread id
   * @param model_id            model id
   * @param sender_queue        the work queue of a sender communication thread
   * @param partition_manager   model partition manager
   * @param callback_runner     callback runner to handle received replies from servers
   */
  KVClientTable(uint32_t app_thread_id, uint32_t model_id, ThreadsafeQueue<Message>* const sender_queue,
                const AbstractPartitionManager* const partition_manager, AbstractCallbackRunner* const callback_runner)
      : app_thread_id_(app_thread_id),
        model_id_(model_id),
        sender_queue_(sender_queue),
        partition_manager_(partition_manager),
        callback_runner_(callback_runner){};

  // ========== API ========== //
  void Clock();
  // vector version
  void Add(const std::vector<Key>& keys, const std::vector<Val>& vals) {
    third_party::SArray<double> vtmp;
    third_party::SArray<Key> ktmp;
    int i=0;
    while(i<vals.size()){
      vtmp.push_back(vals[i]);
      i++;
    }
    i=0;
    while(i<keys.size()){
      ktmp.push_back(keys[i]);
      i++;
    }
    std::vector<std::pair<int,KVPairs>> sliced;
    partition_manager_->Slice(std::make_pair(ktmp,vtmp),&sliced);
    uint32_t count=0;
    while(count<sliced.size()){
      Message m;
      third_party::SArray<char> key_char;
      key_char=sliced[count].second.first;
      third_party::SArray<float> val_chart;
      int j=0;
      while(j<sliced[count].second.second.size()){
        val_chart.push_back((float)sliced[count].second.second.data()[j]);
        j++;
      }
      third_party::SArray<char> val_char;
      val_char=val_chart;
      m.AddData(key_char);
      m.AddData(val_char);
      m.meta.sender=app_thread_id_;
      m.meta.recver=sliced[count].first;
      m.meta.flag=Flag::kAdd;
      m.meta.model_id=model_id_;
      sender_queue_->Push(m);
      count++;
    }
  }
  void Get(const std::vector<Key>& keys, std::vector<Val>* vals) {}
  // sarray version
  void Add(const third_party::SArray<Key>& keys, const third_party::SArray<Val>& vals) {}
  void Get(const third_party::SArray<Key>& keys, third_party::SArray<Val>* vals) {}
  // ========== API ========== //

 private:
  uint32_t app_thread_id_;  // identifies the user thread
  uint32_t model_id_;       // identifies the model on servers

  ThreadsafeQueue<Message>* const sender_queue_;             // not owned
  AbstractCallbackRunner* const callback_runner_;            // not owned
  const AbstractPartitionManager* const partition_manager_;  // not owned

};  // class KVClientTable

}  // namespace csci5570
