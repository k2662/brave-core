import * as React from 'react'
import { api } from '../../../../brave_news/browser/resources/context'
import usePromise from '$web-common/usePromise'
import CardLoading from './cards/cardLoading'

const FeedItems = React.lazy(() => import('../../../../brave_news/browser/resources/Feed'))

export const FeedV2 = () => {
  const { result: feed } = usePromise(() => api.getFeedV2(), [])

  return <React.Suspense fallback={<CardLoading />}>
    <FeedItems feed={feed?.feed} />
  </React.Suspense>
}
