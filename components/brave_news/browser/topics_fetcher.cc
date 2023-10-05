// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/topics_fetcher.h"

#include <utility>
#include <variant>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/api/topics.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/urls.h"

namespace brave_news {

namespace {

std::vector<Topic> ParseTopics(const base::Value& topics_json,
                               const base::Value& topic_articles_json) {
  std::vector<Topic> result;
  base::flat_map<int, std::vector<TopicArticle>> articles;

  if (auto* list = topic_articles_json.GetIfList()) {
    for (const auto& a : *list) {
      auto article_raw = api::topics::TopicArticle::FromValue(a);
      if (!article_raw.has_value()) {
        LOG(ERROR) << "Failed to parse article: " << article_raw.error();
        LOG(ERROR) << "JSON: " << a;
        continue;
      }
      TopicArticle article;
      article.title = article_raw->title;
      article.category = article_raw->category;
      if (article_raw->description) {
        article.description = article_raw->description.value();
      }
      article.url = GURL(article_raw->url);
      if (article_raw->img) {
        article.img = GURL(article_raw->img.value());
      }
      article.origin = article_raw->origin;
      //   article.publish_time = article_raw->publish_time;
      article.score = article_raw->score;

      articles[article_raw->topic_index].push_back(std::move(article));
    }
  } else {
    LOG(ERROR) << "topic articles response was not a list!";
  }

  if (auto* list = topics_json.GetIfList()) {
    for (const auto& t : *list) {
      auto topic_raw = api::topics::Topic::FromValue(t);
      if (!topic_raw.has_value()) {
        LOG(ERROR) << "Failed to parse topic: " << topic_raw.error();
        continue;
      }
      Topic topic;
      topic.title = topic_raw->title;
      topic.claude_title = topic_raw->claude_title;
      topic.claude_title_short = topic_raw->claude_title_short;
      topic.breaking_score = topic_raw->breaking_score;
      topic.overall_score = topic_raw->overall_score;
      topic.most_popular_query = topic_raw->most_popular_query;
      topic.queries = topic_raw->queries;
      //   topic.timestamp = topic_raw->timestamp;

      // There should only be one topic for a set of topic_articles.
      auto it = articles.find(topic_raw->topic_index);
      if (it != articles.end()) {
        topic.articles = std::move(it->second);
      }

      result.push_back(std::move(topic));
    }
  } else {
    LOG(ERROR) << "topics response was not a list: " << topics_json;
  }

  return result;
}

}  // namespace

TopicArticle::TopicArticle() = default;
TopicArticle::~TopicArticle() = default;
TopicArticle::TopicArticle(const TopicArticle&) = default;
TopicArticle::TopicArticle(TopicArticle&&) = default;
TopicArticle& TopicArticle::operator=(const TopicArticle&) = default;

Topic::Topic() = default;
Topic::~Topic() = default;
Topic::Topic(const Topic&) = default;
Topic::Topic(Topic&&) = default;
Topic& Topic::operator=(const Topic&) = default;

TopicsFetcher::FetchState::FetchState(std::string locale,
                                      TopicsCallback callback)
    : locale(locale), callback(std::move(callback)) {}
TopicsFetcher::FetchState::~FetchState() = default;
TopicsFetcher::FetchState::FetchState(FetchState&&) = default;

TopicsFetcher::TopicsFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

TopicsFetcher::~TopicsFetcher() = default;

void TopicsFetcher::GetTopics(const std::string& locale,
                              TopicsCallback callback) {
  FetchTopics(FetchState(locale, std::move(callback)));
}

void TopicsFetcher::FetchTopics(FetchState state) {
  GURL url(base::StrCat({"https://", brave_news::GetHostname(), kTopicsEndpoint,
                         ".", state.locale, ".json"}));
  api_request_helper_.Request(
      "GET", url, "", "",
      base::BindOnce(&TopicsFetcher::OnFetchedTopics, base::Unretained(this),
                     std::move(state)));
}

void TopicsFetcher::OnFetchedTopics(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    LOG(ERROR) << "Failed to get topics: " << result.error_code() << ", "
               << result.body();
    std::move(state.callback).Run({});
    return;
  }
  // TODO: Don't clone
  state.topics_json = result.value_body().Clone();
  FetchTopicArticles(std::move(state));
}

void TopicsFetcher::FetchTopicArticles(FetchState state) {
  GURL url(base::StrCat({"https://", brave_news::GetHostname(),
                         kTopicArticlesEndpoint, ".", state.locale, ".json"}));
  api_request_helper_.Request(
      "GET", url, "", "",
      base::BindOnce(&TopicsFetcher::OnFetchedTopicArticles,
                     base::Unretained(this), std::move(state)));
}

void TopicsFetcher::OnFetchedTopicArticles(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    LOG(ERROR) << "Failed to get topic articles: " << result.error_code()
               << ", " << result.body();
    std::move(state.callback).Run({});
    return;
  }
  // TODO: Don't clone
  state.topic_articles_json = result.value_body().Clone();

  auto topics = ParseTopics(state.topics_json, state.topic_articles_json);
  std::move(state.callback).Run(std::move(topics));
}

}  // namespace brave_news
