package main

import (
	"context"
	"log"
	"time"

	"socialnetwork/gen-go/social_network"
	"github.com/opentracing/opentracing-go"
)


type ComposePostServiceHander struct {
	text_client_pool          *ThriftClientPool
	user_client_pool          *ThriftClientPool
	media_client_pool         *ThriftClientPool
	unique_id_client_pool     *ThriftClientPool
	post_storage_client_pool  *ThriftClientPool
	user_timeline_client_pool *ThriftClientPool
	home_timeline_client_pool *ThriftClientPool
}

func NewComposePostServiceHander(
	_text_client_pool          *ThriftClientPool,
	_user_client_pool          *ThriftClientPool,
	_media_client_pool         *ThriftClientPool,
	_unique_id_client_pool     *ThriftClientPool,
	_post_storage_client_pool  *ThriftClientPool,
	_user_timeline_client_pool *ThriftClientPool,
	_home_timeline_client_pool *ThriftClientPool,
) *ComposePostServiceHander {
	return &ComposePostServiceHander{
		text_client_pool: _text_client_pool,
		user_client_pool: _user_client_pool,
		media_client_pool: _media_client_pool,
		unique_id_client_pool: _unique_id_client_pool,
		post_storage_client_pool: _post_storage_client_pool,
		user_timeline_client_pool: _user_timeline_client_pool,
		home_timeline_client_pool: _home_timeline_client_pool,
	}
}

func (cs *ComposePostServiceHander) _ComposeTextHelper(ctx context.Context, req_id int64, text string, carrier map[string]string, return_text chan *social_network.TextServiceReturn, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("compose_text_client")
	} else {
		span = tracer.StartSpan("compose_text_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get text client
	tc := cs.text_client_pool.Get()
	text_client := social_network.NewTextServiceClient(tc.GetClient())
	_return_text, _err := text_client.ComposeText(ctx, req_id, text, writer)
	if _err != nil {
		log.Printf("Failed to send compose-text to text-service:%s", _err.Error())
		tc.Disconnect()
	} else {
		cs.text_client_pool.Push(tc)
	}
	return_text <- _return_text
	err <- _err
}

func (cs *ComposePostServiceHander) _ComposeCreaterHelper(ctx context.Context, req_id int64, user_id int64, username string, carrier map[string]string, return_creator chan *social_network.Creator, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("compose_creator_client")
	} else {
		span = tracer.StartSpan("compose_creator_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get user client
	tc := cs.user_client_pool.Get()
	user_client := social_network.NewUserServiceClient(tc.GetClient())
	_return_creator, _err := user_client.ComposeCreatorWithUserId(ctx, req_id, user_id, username, writer)
	if _err != nil {
		log.Print("Failed to send compose-creater to user-service")
		tc.Disconnect()
	} else {
		cs.user_client_pool.Push(tc)
	}
	return_creator <- _return_creator
	err <- _err
}

func (cs *ComposePostServiceHander) _ComposeMediaHelper(ctx context.Context, req_id int64, media_types []string, media_ids []int64, carrier map[string]string, return_media chan []*social_network.Media, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("compose_media_client")
	} else {
		span = tracer.StartSpan("compose_media_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get media client
	tc := cs.media_client_pool.Get()
	media_client := social_network.NewMediaServiceClient(tc.GetClient())
	_return_media, _err := media_client.ComposeMedia(ctx, req_id, media_types, media_ids, writer)
	if _err != nil {
		log.Print("Failed to send compose-media to media-service")
		tc.Disconnect()
	} else {
		cs.media_client_pool.Push(tc)
	}
	return_media <- _return_media
	err <- _err
}

func (cs *ComposePostServiceHander) _ComposeUniqueIdHelper(ctx context.Context, req_id int64, post_type social_network.PostType, carrier map[string]string, return_unique_id chan int64, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("compose_unique_id_client")
	} else {
		span = tracer.StartSpan("compose_unique_id_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get unique id client
	tc := cs.unique_id_client_pool.Get()
	unique_id_client := social_network.NewUniqueIdServiceClient(tc.GetClient())
	_return_unique_id, _err := unique_id_client.ComposeUniqueId(ctx, req_id, post_type, writer)
	if _err != nil {
		log.Printf("Failed to send compose-unique-id to unique-id-service")
		tc.Disconnect()
	} else {
		cs.unique_id_client_pool.Push(tc)
	}
	return_unique_id <- _return_unique_id
	err <- _err
}

func (cs *ComposePostServiceHander) _UploadPostHelper(ctx context.Context, req_id int64, post *social_network.Post, carrier map[string]string, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("store_post_client")
	} else {
		span = tracer.StartSpan("store_post_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get post storage client
	tc := cs.post_storage_client_pool.Get()
	post_storage_client := social_network.NewPostStorageServiceClient(tc.GetClient())
	_err = post_storage_client.StorePost(ctx, req_id, post, writer)
	if _err != nil {
		log.Print("Failed to store post to post-storage-service")
		tc.Disconnect()
	} else {
		cs.post_storage_client_pool.Push(tc)
	}
	err <- _err
}

func (cs *ComposePostServiceHander) _UploadUserTimelineHelper(ctx context.Context, req_id int64, post_id int64, user_id int64, timestamp int64, carrier map[string]string, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("write_user_timeline_client")
	} else {
		span = tracer.StartSpan("write_user_timeline_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get user timeline client
	tc := cs.user_timeline_client_pool.Get()
	user_timeline_client := social_network.NewUserTimelineServiceClient(tc.GetClient())
	_err = user_timeline_client.WriteUserTimeline(ctx, req_id, post_id, user_id, timestamp, writer)
	if _err != nil {
		log.Print("Failed to write user timeline to user-timeline-service")
		tc.Disconnect()
	} else {
		cs.user_timeline_client_pool.Push(tc)
	}
	err <- _err
}

func (cs *ComposePostServiceHander) _UploadHomeTimelineHelper(ctx context.Context, req_id int64, post_id int64, user_id int64, timestamp int64, user_mentions_id []int64, carrier map[string]string, err chan error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, _err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if _err != nil {
		span = tracer.StartSpan("write_home_timeline_client")
	} else {
		span = tracer.StartSpan("write_home_timeline_client", opentracing.ChildOf(parent_span))
	}
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// get home timeline client
	tc := cs.home_timeline_client_pool.Get()
	home_timeline_client := social_network.NewHomeTimelineServiceClient(tc.GetClient())
	_err = home_timeline_client.WriteHomeTimeline(ctx, req_id, post_id, user_id, timestamp, user_mentions_id, writer)
	if _err != nil {
		log.Print("Failed to write home timeline to home-timeline-service")
		tc.Disconnect()
	} else {
		cs.home_timeline_client_pool.Push(tc)
	}
	err <- _err
}

func (cps *ComposePostServiceHander) ComposePost(ctx context.Context, req_id int64, username string, user_id int64, text string, media_ids []int64, media_types []string, post_type social_network.PostType, carrier map[string]string) (error) {
	// TODO init tracer
	var span opentracing.Span
	tracer := opentracing.GlobalTracer()
	parent_span, err := tracer.Extract(opentracing.TextMap, opentracing.TextMapCarrier(carrier))
	if err != nil {
		span = tracer.StartSpan("compose_post_server")
	} else {
		span = tracer.StartSpan("compose_post_server", opentracing.ChildOf(parent_span))
	}
	//var writer TextMapWriter
	writer := make(opentracing.TextMapCarrier)
	tracer.Inject(span.Context(), opentracing.TextMap, writer)
	defer span.Finish()

	// async call text helper
	return_text := make(chan *social_network.TextServiceReturn)
	err_text := make(chan error)
	go cps._ComposeTextHelper(ctx, req_id, text, writer, return_text, err_text)
	// async call creator helper
	return_creator := make(chan *social_network.Creator)
	err_creator := make(chan error)
	go cps._ComposeCreaterHelper(ctx, req_id, user_id, username, writer, return_creator, err_creator)
	// async call media helper
	return_media := make(chan []*social_network.Media)
	err_media := make(chan error)
	go cps._ComposeMediaHelper(ctx, req_id, media_types, media_ids, writer, return_media, err_media)
	// async call unique id helper
	return_unique_id := make(chan int64)
	err_unique_id := make(chan error)
	go cps._ComposeUniqueIdHelper(ctx, req_id, post_type, writer, return_unique_id, err_unique_id)
	// get post data from async calls

	var post social_network.Post
	timestamp := time.Now().UnixMilli()
	post.Timestamp = timestamp
	post.PostID = <- return_unique_id
	err = <- err_unique_id
	if err != nil {
		log.Print("Failed to get unique id service return")
	}
	post.Creator = <- return_creator
	err = <- err_creator
	if err != nil {
		log.Print("Failed to get creater service return")
	}
	post.Media = <-return_media
	err = <- err_media
	if err != nil {
		log.Print("Failed to get media service return")
	}
	texts := <- return_text
	err = <- err_text
	if err != nil {
		log.Print("failed to get text service return")
	}
	post.Text = texts.GetText()
	post.Urls = texts.GetUrls()
	post.UserMentions = texts.GetUserMentions()
	post.ReqID = req_id
	post.PostType = post_type
	var user_mention_ids []int64
	for _, item := range post.UserMentions {
		user_mention_ids = append(user_mention_ids, item.UserID)
	}
	// async call upload post helper
	err_upload_post := make(chan error)
	go cps._UploadPostHelper(ctx, req_id, &post, writer, err_upload_post)
	// async call upload user timeline helper
	err_upload_user_timeline := make(chan error)
	go cps._UploadUserTimelineHelper(ctx, req_id, post.PostID, user_id, timestamp, writer, err_upload_user_timeline)
	// async call upload home timeline helper
	err_upload_home_timeline := make(chan error)
	go cps._UploadHomeTimelineHelper(ctx, req_id, post.PostID, user_id, timestamp, user_mention_ids, writer, err_upload_home_timeline)
	// wait for async call to finish
	err = <- err_upload_post
	if err != nil {
		log.Print("failed to upload post")
	}
	err = <- err_upload_user_timeline
	if err != nil {
		log.Print("failed to upload user timeline")
	}
	err = <- err_upload_home_timeline
	if err != nil {
		log.Print("failed to upload home timeline")
	}
	return nil
}
