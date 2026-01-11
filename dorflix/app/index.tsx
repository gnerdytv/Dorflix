import React, { useRef, useState } from "react";
import { FlatList, Dimensions } from "react-native";
import VideoCard from "../components/VideoCard";

const { height } = Dimensions.get("window");

interface VideoData {
  id: string;
  VideoUri: string;
  title: string;
  likes: string;
  comments: number;
  shares: number;
}

const videos: VideoData[] = [
  {
    id: "0",
    VideoUri:
      "https://res.cloudinary.com/dtdupsyqm/video/upload/v1766508895/7584372777688370488_original_k2kwhr.mp4",
    title: "Baby, We can go there",
    likes: "12.3K",
    comments: 485,
    shares: 102,
  },
  {
    id: "1",
    VideoUri:
      "https://res.cloudinary.com/dtdupsyqm/video/upload/v1766509668/7584984055100280075_original_olppti.mp4",
    title: "Omor",
    likes: "8.1K",
    comments: 230,
    shares: 56,
  },
  {
    id: "2",
    VideoUri:
      "https://res.cloudinary.com/dtdupsyqm/video/upload/v1766509866/7586391685144300812_original_wgwdac.mp4",
    title: "I wanna know you more",
    likes: "10.1K",
    comments: 700,
    shares: 5,
  },
  {
    id: "3",
    VideoUri:
      "https://res.cloudinary.com/dtdupsyqm/video/upload/v1766526532/7568162860883381560_original_trnpu7_ddkyys.mp4",
    title: "I'm going no where'",
    likes: "10.1K",
    comments: 700,
    shares: 5,
  },
];

export default function Home() {
  const [currentIndex, setCurrentIndex] = useState(0);

  const viewabilityConfig = useRef({
    itemVisiblePercentThreshold: 80,
  });

  const onViewableItemsChanged = useRef(
    ({ viewableItems }: { viewableItems: any[] }) => {
      if (!viewableItems.length) return;

      // Pick the most visible item
      const mostVisible = viewableItems.reduce((prev, curr) =>
        (curr?.visibilityPercent ?? 0) > (prev?.visibilityPercent ?? 0)
          ? curr
          : prev
      );

      if (mostVisible?.index != null) {
        setCurrentIndex(mostVisible.index);
      }
    }
  );

  return (
    <FlatList
      data={videos}
      keyExtractor={(item) => item.id}
      renderItem={({ item, index }) => (
        <VideoCard
          videoUri={item.VideoUri}
          title={item.title}
          likes={item.likes}
          comments={item.comments}
          shares={item.shares}
          isActive={index === currentIndex}
        />
      )}
      pagingEnabled
      showsVerticalScrollIndicator={false}
      removeClippedSubviews={false}
      onViewableItemsChanged={onViewableItemsChanged.current}
      viewabilityConfig={viewabilityConfig.current}
      getItemLayout={(_, index) => ({
        length: height,
        offset: height * index,
        index,
      })}
      initialNumToRender={1}
      maxToRenderPerBatch={1}
      windowSize={2}
    />
  );
}
