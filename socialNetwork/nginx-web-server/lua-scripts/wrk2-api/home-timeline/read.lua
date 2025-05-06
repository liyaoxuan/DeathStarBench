local _M = {}
local k8s_suffix = os.getenv("fqdn_suffix")
if (k8s_suffix == nil) then
  k8s_suffix = ""
end

-- local socket = require "socket"
local ffi = require("ffi")

-- 定义系统调用
ffi.cdef[[
    typedef struct timespec {
        long tv_sec;
        long tv_nsec;
    } timespec;
    int clock_gettime(int clockid, struct timespec *tp);
]]

-- 获取毫秒级时间戳
local function get_time_ns()
    local tv = ffi.new("timespec")
    ffi.C.clock_gettime(1, tv)
    return tonumber(tv.tv_sec) * 1000000000 + tonumber(tv.tv_nsec)
end

local function _StrIsEmpty(s)
  return s == nil or s == ''
end

local function _LoadTimeline(data)
  local timeline = {}
  for _, timeline_post in ipairs(data) do
    local new_post = {}
    new_post["post_id"] = tostring(timeline_post.post_id)
    new_post["creator"] = {}
    new_post["creator"]["user_id"] = tostring(timeline_post.creator.user_id)
    new_post["creator"]["username"] = timeline_post.creator.username
    new_post["req_id"] = tostring(timeline_post.req_id)
    new_post["text"] = timeline_post.text
    new_post["user_mentions"] = {}
    for _, user_mention in ipairs(timeline_post.user_mentions) do
      local new_user_mention = {}
      new_user_mention["user_id"] = tostring(user_mention.user_id)
      new_user_mention["username"] = user_mention.username
      table.insert(new_post["user_mentions"], new_user_mention)
    end
    new_post["media"] = {}
    for _, media in ipairs(timeline_post.media) do
      local new_media = {}
      new_media["media_id"] = tostring(media.media_id)
      new_media["media_type"] = media.media_type
      table.insert(new_post["media"], new_media)
    end
    new_post["urls"] = {}
    for _, url in ipairs(timeline_post.urls) do
      local new_url = {}
      new_url["shortened_url"] = url.shortened_url
      new_url["expanded_url"] = url.expanded_url
      table.insert(new_post["urls"], new_url)
    end
    new_post["timestamp"] = tostring(timeline_post.timestamp)
    new_post["post_type"] = timeline_post.post_type
    table.insert(timeline, new_post)
  end
  return timeline
end

function _M.ReadHomeTimeline()
  local bridge_tracer = require "opentracing_bridge_tracer"
  local ngx = ngx
  local GenericObjectPool = require "GenericObjectPool"
  local social_network_HomeTimelineService = require "social_network_HomeTimelineService"
  local HomeTimelineServiceClient = social_network_HomeTimelineService.HomeTimelineServiceClient
  local cjson = require "cjson"
  local liblualongnumber = require "liblualongnumber"

  local req_id = tonumber(string.sub(ngx.var.request_id, 0, 15), 16)
  local tracer = bridge_tracer.new_from_global()
  local parent_span_context = tracer:binary_extract(
      ngx.var.opentracing_binary_context)

  local span = tracer:start_span("read_home_timeline_client",
      { ["references"] = { { "child_of", parent_span_context } } })
  local carrier = {}
  tracer:text_map_inject(span:context(), carrier)

  ngx.req.read_body()
  local args = ngx.req.get_uri_args()

  if (_StrIsEmpty(args.user_id) or _StrIsEmpty(args.start) or _StrIsEmpty(args.stop) or _StrIsEmpty(args.sla)) then
    ngx.status = ngx.HTTP_BAD_REQUEST
    ngx.say("Incomplete arguments")
    ngx.log(ngx.ERR, "Incomplete arguments")
    ngx.exit(ngx.HTTP_BAD_REQUEST)
  end

  local carrier = {}
  tracer:text_map_inject(span:context(), carrier)

  local context = {}
  local enable = 0
  local sla = 10000000
  local reqid = 0
  if (not _StrIsEmpty(args.enable)) then
    enable = tonumber(args.enable)
  end
  if (not _StrIsEmpty(args.sla)) then
    sla = tonumber(args.sla)
  end
  if (not _StrIsEmpty(args.reqid)) then
    reqid = tonumber(args.reqid)
  end
  context["sched-enable"] = enable
  context["sched-sla"] = sla
  context["sched-time-next"] = 0
  context["sched-time-remaining"] = 0
  -- context["sched-time-start"] = math.floor(socket.gettime() * 1000)
  context["sched-time-start"] = math.floor(get_time_ns())
  context["req-id"] = reqid


  local client = GenericObjectPool:connection(
      HomeTimelineServiceClient, "home-timeline-service" .. k8s_suffix, 9090)
  local status, ret = pcall(client.ReadHomeTimeline, client, req_id,
      tonumber(args.user_id), tonumber(args.start), tonumber(args.stop), carrier, context)
  if not status then
    ngx.status = ngx.HTTP_INTERNAL_SERVER_ERROR
    if (ret.message) then
      ngx.say("Get home-timeline failure: " .. ret.message)
      ngx.log(ngx.ERR, "Get home-timeline failure: " .. ret.message)
    else
      ngx.say("Get home-timeline failure: " .. ret)
      ngx.log(ngx.ERR, "Get home-timeline failure: " .. ret)
    end
    client.iprot.trans:close()
    span:finish()
    ngx.exit(ngx.HTTP_INTERNAL_SERVER_ERROR)
  else
    GenericObjectPool:returnConnection(client)
    local home_timeline = _LoadTimeline(ret)
    ngx.header.content_type = "application/json; charset=utf-8"
    ngx.say(cjson.encode(home_timeline) )
  end
  span:finish()
  ngx.exit(ngx.HTTP_OK)
end

return _M