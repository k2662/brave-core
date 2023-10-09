// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { FeedV2 } from "gen/brave/components/brave_news/common/brave_news.mojom.m";
import * as React from 'react';
import styled from "styled-components";
import Advert from "./feed/Ad";
import Article from "./feed/Article";
import Cluster from "./feed/Cluster";
import Discover from "./feed/Discover";
import HeroArticle from "./feed/Hero";

const FeedContainer = styled.div`
  max-width: 540px;
  display: flex;
  flex-direction: column;
  gap: 12px;
`


interface Props {
  feed: FeedV2 | undefined;
}

export default function Component({ feed }: Props) {
  // For now, we just hardcode the feed size - in future this should scroll till
  // we run out of items.
  const MAX_SIZE_TEMP = 250;
  return <FeedContainer>
    {feed?.items.slice(0, MAX_SIZE_TEMP).map((item, index) => {
      if (item.advert) {
        return <Advert info={item.advert} key={index} />
      }
      if (item.article) {
        return <Article info={item.article} key={item.article.data.url.url} />
      }
      if (item.cluster) {
        return <Cluster info={item.cluster} key={index} />
      }
      if (item.discover) {
        return <Discover info={item.discover} key={index} />
      }
      if (item.hero) {
        return <HeroArticle info={item.hero} key={item.hero.data.url.url} />
      }

      throw new Error("Invalid item!" + JSON.stringify(item))
    })}
  </FeedContainer>
}
