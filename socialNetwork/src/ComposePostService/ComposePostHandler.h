#ifndef SOCIAL_NETWORK_MICROSERVICES_SRC_COMPOSEPOSTSERVICE_COMPOSEPOSTHANDLER_H_
#define SOCIAL_NETWORK_MICROSERVICES_SRC_COMPOSEPOSTSERVICE_COMPOSEPOSTHANDLER_H_

#include <sys/syscall.h>
#include <chrono>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../../gen-cpp/ComposePostService.h"
#include "../../gen-cpp/HomeTimelineService.h"
#include "../../gen-cpp/MediaService.h"
#include "../../gen-cpp/PostStorageService.h"
#include "../../gen-cpp/TextService.h"
#include "../../gen-cpp/UniqueIdService.h"
#include "../../gen-cpp/UserService.h"
#include "../../gen-cpp/UserTimelineService.h"
#include "../../gen-cpp/social_network_types.h"
#include "../ClientPool.h"
#include "../ThriftClient.h"
#include "../logger.h"
#include "../tracing.h"

namespace social_network {
using json = nlohmann::json;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

class ComposePostHandler : public ComposePostServiceIf {
 public:
  ComposePostHandler(ClientPool<ThriftClient<PostStorageServiceClient>> *,
                     ClientPool<ThriftClient<UserTimelineServiceClient>> *,
                     ClientPool<ThriftClient<UserServiceClient>> *,
                     ClientPool<ThriftClient<UniqueIdServiceClient>> *,
                     ClientPool<ThriftClient<MediaServiceClient>> *,
                     ClientPool<ThriftClient<TextServiceClient>> *,
                     ClientPool<ThriftClient<HomeTimelineServiceClient>> *);
  ~ComposePostHandler() override = default;

  void ComposePost(int64_t req_id, const std::string &username, int64_t user_id,
                   const std::string &text,
                   const std::vector<int64_t> &media_ids,
                   const std::vector<std::string> &media_types,
                   PostType::type post_type,
                   const std::map<std::string, std::string> &carrier) override;

 private:
  ClientPool<ThriftClient<PostStorageServiceClient>> *_post_storage_client_pool;
  ClientPool<ThriftClient<UserTimelineServiceClient>>
      *_user_timeline_client_pool;

  ClientPool<ThriftClient<UserServiceClient>> *_user_service_client_pool;
  ClientPool<ThriftClient<UniqueIdServiceClient>>
      *_unique_id_service_client_pool;
  ClientPool<ThriftClient<MediaServiceClient>> *_media_service_client_pool;
  ClientPool<ThriftClient<TextServiceClient>> *_text_service_client_pool;
  ClientPool<ThriftClient<HomeTimelineServiceClient>>
      *_home_timeline_client_pool;

  void _UploadUserTimelineHelper(
      int64_t req_id, int64_t post_id, int64_t user_id, int64_t timestamp,
      const std::map<std::string, std::string> &carrier);

  void _UploadPostHelper(int64_t req_id, const Post &post,
                         const std::map<std::string, std::string> &carrier);

  void _UploadHomeTimelineHelper(
      int64_t req_id, int64_t post_id, int64_t user_id, int64_t timestamp,
      const std::vector<int64_t> &user_mentions_id,
      const std::map<std::string, std::string> &carrier);

  Creator _ComposeCreaterHelper(
      int64_t req_id, int64_t user_id, const std::string &username,
      const std::map<std::string, std::string> &carrier);
  TextServiceReturn _ComposeTextHelper(
      int64_t req_id, const std::string &text,
      const std::map<std::string, std::string> &carrier);
  std::vector<Media> _ComposeMediaHelper(
      int64_t req_id, const std::vector<std::string> &media_types,
      const std::vector<int64_t> &media_ids,
      const std::map<std::string, std::string> &carrier);
  int64_t _ComposeUniqueIdHelper(
      int64_t req_id, PostType::type post_type,
      const std::map<std::string, std::string> &carrier);
};

ComposePostHandler::ComposePostHandler(
    ClientPool<social_network::ThriftClient<PostStorageServiceClient>>
        *post_storage_client_pool,
    ClientPool<social_network::ThriftClient<UserTimelineServiceClient>>
        *user_timeline_client_pool,
    ClientPool<ThriftClient<UserServiceClient>> *user_service_client_pool,
    ClientPool<ThriftClient<UniqueIdServiceClient>>
        *unique_id_service_client_pool,
    ClientPool<ThriftClient<MediaServiceClient>> *media_service_client_pool,
    ClientPool<ThriftClient<TextServiceClient>> *text_service_client_pool,
    ClientPool<ThriftClient<HomeTimelineServiceClient>>
        *home_timeline_client_pool) {
  _post_storage_client_pool = post_storage_client_pool;
  _user_timeline_client_pool = user_timeline_client_pool;
  _user_service_client_pool = user_service_client_pool;
  _unique_id_service_client_pool = unique_id_service_client_pool;
  _media_service_client_pool = media_service_client_pool;
  _text_service_client_pool = text_service_client_pool;
  _home_timeline_client_pool = home_timeline_client_pool;
}

Creator ComposePostHandler::_ComposeCreaterHelper(
    int64_t req_id, int64_t user_id, const std::string &username,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_creator_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto user_client_wrapper = _user_service_client_pool->Pop();
  if (!user_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to user-service";
    LOG(error) << se.message;
    span->Finish();
    throw se;
  }

  auto user_client = user_client_wrapper->GetClient();
  Creator _return_creator;
  try {
    user_client->ComposeCreatorWithUserId(_return_creator, req_id, user_id,
                                          username, writer_text_map);
  } catch (...) {
    LOG(error) << "Failed to send compose-creator to user-service";
    _user_service_client_pool->Remove(user_client_wrapper);
    span->Finish();
    throw;
  }
  _user_service_client_pool->Keepalive(user_client_wrapper);
  span->Finish();
  return _return_creator;
}

TextServiceReturn ComposePostHandler::_ComposeTextHelper(
    int64_t req_id, const std::string &text,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_text_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto text_client_wrapper = _text_service_client_pool->Pop();
  if (!text_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to text-service";
    LOG(error) << se.message;
    ;
    span->Finish();
    throw se;
  }

  auto text_client = text_client_wrapper->GetClient();
  TextServiceReturn _return_text;
  try {
    text_client->ComposeText(_return_text, req_id, text, writer_text_map);
  } catch (...) {
    LOG(error) << "Failed to send compose-text to text-service";
    _text_service_client_pool->Remove(text_client_wrapper);
    span->Finish();
    throw;
  }
  _text_service_client_pool->Keepalive(text_client_wrapper);
  span->Finish();
  return _return_text;
}

std::vector<Media> ComposePostHandler::_ComposeMediaHelper(
    int64_t req_id, const std::vector<std::string> &media_types,
    const std::vector<int64_t> &media_ids,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_media_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto media_client_wrapper = _media_service_client_pool->Pop();
  if (!media_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to media-service";
    LOG(error) << se.message;
    ;
    span->Finish();
    throw se;
  }

  auto media_client = media_client_wrapper->GetClient();
  std::vector<Media> _return_media;
  try {
    media_client->ComposeMedia(_return_media, req_id, media_types, media_ids,
                               writer_text_map);
  } catch (...) {
    LOG(error) << "Failed to send compose-media to media-service";
    _media_service_client_pool->Remove(media_client_wrapper);
    span->Finish();
    throw;
  }
  _media_service_client_pool->Keepalive(media_client_wrapper);
  span->Finish();
  return _return_media;
}

int64_t ComposePostHandler::_ComposeUniqueIdHelper(
    int64_t req_id, const PostType::type post_type,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_unique_id_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto unique_id_client_wrapper = _unique_id_service_client_pool->Pop();
  if (!unique_id_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to unique_id-service";
    LOG(error) << se.message;
    span->Finish();
    throw se;
  }

  auto unique_id_client = unique_id_client_wrapper->GetClient();
  int64_t _return_unique_id;
  try {
    _return_unique_id =
        unique_id_client->ComposeUniqueId(req_id, post_type, writer_text_map);
  } catch (...) {
    LOG(error) << "Failed to send compose-unique_id to unique_id-service";
    _unique_id_service_client_pool->Remove(unique_id_client_wrapper);
    span->Finish();
    throw;
  }
  _unique_id_service_client_pool->Keepalive(unique_id_client_wrapper);
  span->Finish();
  return _return_unique_id;
}

void ComposePostHandler::_UploadPostHelper(
    int64_t req_id, const Post &post,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "store_post_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto post_storage_client_wrapper = _post_storage_client_pool->Pop();
  if (!post_storage_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to post-storage-service";
    LOG(error) << se.message;
    ;
    throw se;
  }
  auto post_storage_client = post_storage_client_wrapper->GetClient();
  try {
    post_storage_client->StorePost(req_id, post, writer_text_map);
  } catch (...) {
    _post_storage_client_pool->Remove(post_storage_client_wrapper);
    LOG(error) << "Failed to store post to post-storage-service";
    throw;
  }
  _post_storage_client_pool->Keepalive(post_storage_client_wrapper);

  span->Finish();
}

void ComposePostHandler::_UploadUserTimelineHelper(
    int64_t req_id, int64_t post_id, int64_t user_id, int64_t timestamp,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "write_user_timeline_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto user_timeline_client_wrapper = _user_timeline_client_pool->Pop();
  if (!user_timeline_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to user-timeline-service";
    LOG(error) << se.message;
    ;
    throw se;
  }
  auto user_timeline_client = user_timeline_client_wrapper->GetClient();
  try {
    user_timeline_client->WriteUserTimeline(req_id, post_id, user_id, timestamp,
                                            writer_text_map);
  } catch (...) {
    _user_timeline_client_pool->Remove(user_timeline_client_wrapper);
    throw;
  }
  _user_timeline_client_pool->Keepalive(user_timeline_client_wrapper);

  span->Finish();
}

void ComposePostHandler::_UploadHomeTimelineHelper(
    int64_t req_id, int64_t post_id, int64_t user_id, int64_t timestamp,
    const std::vector<int64_t> &user_mentions_id,
    const std::map<std::string, std::string> &carrier) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "write_home_timeline_client",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto home_timeline_client_wrapper = _home_timeline_client_pool->Pop();
  if (!home_timeline_client_wrapper) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_CONN_ERROR;
    se.message = "Failed to connect to home-timeline-service";
    LOG(error) << se.message;
    ;
    throw se;
  }
  auto home_timeline_client = home_timeline_client_wrapper->GetClient();
  try {
    home_timeline_client->WriteHomeTimeline(req_id, post_id, user_id, timestamp,
                                            user_mentions_id, writer_text_map);
  } catch (...) {
    _home_timeline_client_pool->Remove(home_timeline_client_wrapper);
    LOG(error) << "Failed to write home timeline to home-timeline-service";
    throw;
  }
  _home_timeline_client_pool->Keepalive(home_timeline_client_wrapper);

  span->Finish();
}

void ComposePostHandler::ComposePost(
    const int64_t req_id, const std::string &username, int64_t user_id,
    const std::string &text, const std::vector<int64_t> &media_ids,
    const std::vector<std::string> &media_types, const PostType::type post_type,
    const std::map<std::string, std::string> &carrier) {
  TextMapReader reader(carrier);
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto tid = syscall(__NR_gettid);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_post_server",
      {opentracing::ChildOf(parent_span->get()),
      opentracing::SetTag{"time", t},
      opentracing::SetTag{"tid", tid}});
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  auto text_future =
      std::async(std::launch::async, &ComposePostHandler::_ComposeTextHelper,
                 this, req_id, text, writer_text_map);

  TextMapReader reader_1(writer_text_map);
  auto parent_span_1 = opentracing::Tracer::Global()->Extract(reader_1);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_1 = opentracing::Tracer::Global()->StartSpan(
      "async_creater",
      {opentracing::ChildOf(parent_span_1->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_1;
  TextMapWriter writer_1(writer_text_map_1);
  opentracing::Tracer::Global()->Inject(span->context(), writer_1);
  auto creator_future =
      std::async(std::launch::async, &ComposePostHandler::_ComposeCreaterHelper,
                 this, req_id, user_id, username, writer_text_map);
  span_1->Finish();

  TextMapReader reader_2(writer_text_map);
  auto parent_span_2 = opentracing::Tracer::Global()->Extract(reader_2);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_2 = opentracing::Tracer::Global()->StartSpan(
      "async_media",
      {opentracing::ChildOf(parent_span_2->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_2;
  TextMapWriter writer_2(writer_text_map_2);
  opentracing::Tracer::Global()->Inject(span->context(), writer_2);
  auto media_future =
      std::async(std::launch::async, &ComposePostHandler::_ComposeMediaHelper,
                 this, req_id, media_types, media_ids, writer_text_map);
  span_2->Finish();

  TextMapReader reader_3(writer_text_map);
  auto parent_span_3 = opentracing::Tracer::Global()->Extract(reader_3);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_3 = opentracing::Tracer::Global()->StartSpan(
      "async_unique_id",
      {opentracing::ChildOf(parent_span_3->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_3;
  TextMapWriter writer_3(writer_text_map_3);
  opentracing::Tracer::Global()->Inject(span->context(), writer_3);
  auto unique_id_future = std::async(
      std::launch::async, &ComposePostHandler::_ComposeUniqueIdHelper, this,
      req_id, post_type, writer_text_map);
  span_3->Finish();

  Post post;
  auto timestamp =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count();
  post.timestamp = timestamp;

  // try
  // {
  
  TextMapReader reader_7(writer_text_map);
  auto parent_span_7 = opentracing::Tracer::Global()->Extract(reader_7);
  auto span_7 = opentracing::Tracer::Global()->StartSpan(
      "async_unique_id_get",
      {opentracing::ChildOf(parent_span_7->get()),
      });
  std::map<std::string, std::string> writer_text_map_7;
  TextMapWriter writer_7(writer_text_map_7);
  opentracing::Tracer::Global()->Inject(span->context(), writer_7);
  post.post_id = unique_id_future.get();
  span_7->Finish();

  TextMapReader reader_8(writer_text_map);
  auto parent_span_8 = opentracing::Tracer::Global()->Extract(reader_8);
  auto span_8 = opentracing::Tracer::Global()->StartSpan(
      "async_creator_get",
      {opentracing::ChildOf(parent_span_8->get()),
      });
  std::map<std::string, std::string> writer_text_map_8;
  TextMapWriter writer_8(writer_text_map_8);
  opentracing::Tracer::Global()->Inject(span->context(), writer_8);
  post.creator = creator_future.get();
  span_8->Finish();

  TextMapReader reader_9(writer_text_map);
  auto parent_span_9 = opentracing::Tracer::Global()->Extract(reader_9);
  auto span_9 = opentracing::Tracer::Global()->StartSpan(
      "async_media_get",
      {opentracing::ChildOf(parent_span_9->get()),
      });
  std::map<std::string, std::string> writer_text_map_9;
  TextMapWriter writer_9(writer_text_map_9);
  opentracing::Tracer::Global()->Inject(span->context(), writer_9);
  post.media = media_future.get();
  span_9->Finish();

  TextMapReader reader_10(writer_text_map);
  auto parent_span_10 = opentracing::Tracer::Global()->Extract(reader_10);
  auto span_10 = opentracing::Tracer::Global()->StartSpan(
      "async_text_get",
      {opentracing::ChildOf(parent_span_10->get()),
      });
  std::map<std::string, std::string> writer_text_map_10;
  TextMapWriter writer_10(writer_text_map_10);
  opentracing::Tracer::Global()->Inject(span->context(), writer_10);
  auto text_return = text_future.get();
  span_10->Finish();

  post.text = text_return.text;
  post.urls = text_return.urls;
  post.user_mentions = text_return.user_mentions;
  post.req_id = req_id;
  post.post_type = post_type;
  // }
  // catch (...)
  // {
  //   throw;
  // }

  std::vector<int64_t> user_mention_ids;
  for (auto &item : post.user_mentions) {
    user_mention_ids.emplace_back(item.user_id);
  }

  //In mixed workloed condition, need to make sure _UploadPostHelper execute
  //Before _UploadUserTimelineHelper and _UploadHomeTimelineHelper.
  //Change _UploadUserTimelineHelper and _UploadHomeTimelineHelper to deferred.
  //To let them start execute after post_future.get() return.
  TextMapReader reader_4(writer_text_map);
  auto parent_span_4 = opentracing::Tracer::Global()->Extract(reader_4);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_4 = opentracing::Tracer::Global()->StartSpan(
      "async_upload_post",
      {opentracing::ChildOf(parent_span_4->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_4;
  TextMapWriter writer_4(writer_text_map_4);
  opentracing::Tracer::Global()->Inject(span->context(), writer_4);
  auto post_future = std::async(
      std::launch::async, &ComposePostHandler::_UploadPostHelper, this,
      req_id, post, writer_text_map);
  span_4->Finish();

  TextMapReader reader_5(writer_text_map);
  auto parent_span_5 = opentracing::Tracer::Global()->Extract(reader_5);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_5 = opentracing::Tracer::Global()->StartSpan(
      "async_user_timeline",
      {opentracing::ChildOf(parent_span_5->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_5;
  TextMapWriter writer_5(writer_text_map_5);
  opentracing::Tracer::Global()->Inject(span->context(), writer_5);
  auto user_timeline_future = std::async(
      std::launch::deferred, &ComposePostHandler::_UploadUserTimelineHelper, this,
      req_id, post.post_id, user_id, timestamp, writer_text_map);
  span_5->Finish();

  TextMapReader reader_6(writer_text_map);
  auto parent_span_6 = opentracing::Tracer::Global()->Extract(reader_6);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  t = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
  auto span_6 = opentracing::Tracer::Global()->StartSpan(
      "async_user_timeline",
      {opentracing::ChildOf(parent_span_6->get()),
      opentracing::SetTag{"time", t}});
  std::map<std::string, std::string> writer_text_map_6;
  TextMapWriter writer_6(writer_text_map_6);
  opentracing::Tracer::Global()->Inject(span->context(), writer_6);
  auto home_timeline_future = std::async(
      std::launch::deferred, &ComposePostHandler::_UploadHomeTimelineHelper, this,
      req_id, post.post_id, user_id, timestamp, user_mention_ids,
      writer_text_map);
  span_6->Finish();

  // try
  // {
  post_future.get();
  user_timeline_future.get();
  home_timeline_future.get();
  // }
  // catch (...)
  // {
  //   throw;
  // }
  span->Finish();
}

}  // namespace social_network

#endif  // SOCIAL_NETWORK_MICROSERVICES_SRC_COMPOSEPOSTSERVICE_COMPOSEPOSTHANDLER_H_
