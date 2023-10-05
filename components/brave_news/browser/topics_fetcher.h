// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_

#include <vector>
#include "base/allocator/partition_allocator/pointers/raw_ref.h"
#include "base/functional/callback_forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace brave_news {

constexpr char kTopicsEndpoint[] = "news-topic-clustering/topics";
constexpr char kTopicArticlesEndpoint[] = "news-topic-clustering/topics_news";

struct TopicArticle {
  TopicArticle();
  TopicArticle(const TopicArticle&) = delete;
  TopicArticle& operator=(const TopicArticle&) = delete;
  ~TopicArticle();

  std::string title;
  std::string description;
  GURL url;
  GURL img;
  std::string publisher_name;
  uint64_t publish_time;
  double score;
  std::string category;
  std::string origin;
};

struct Topic {
  Topic();
  Topic(const Topic&) = delete;
  Topic& operator=(const Topic&) = delete;
  ~Topic();

  std::string title;
  std::string claude_title;
  std::string claude_title_short;
  std::string most_popular_query;
  std::vector<std::string> queries;
  uint64_t timestamp;
  double overall_score;
  double breaking_score;

  std::vector<TopicArticle> articles;
};

class TopicsFetcher {
 public:
  using TopicsCallback = base::OnceCallback<void(std::vector<Topic> topics)>;
  explicit TopicsFetcher(
      api_request_helper::APIRequestHelper& api_request_helper);
  TopicsFetcher(const TopicsFetcher&) = delete;
  TopicsFetcher& operator=(const TopicsFetcher&) = delete;
  ~TopicsFetcher();

  void GetTopics(const std::string& locale, TopicsCallback callback);

 private:
  struct FetchState {
    std::string locale;
    TopicsCallback callback;
    std::string topics_json = "";
    std::string topic_articles_json = "";

    FetchState(std::string locale, TopicsCallback callback);
    ~FetchState();

    FetchState(FetchState&&);
  };

  void FetchTopics(FetchState state);
  void OnFetchedTopics(FetchState state,
                       api_request_helper::APIRequestResult result);
  void FetchTopicArticles(FetchState state);
  void OnFetchedTopicArticles(FetchState state,
                              api_request_helper::APIRequestResult result);
  raw_ref<api_request_helper::APIRequestHelper> api_request_helper_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_
