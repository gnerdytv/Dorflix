import { Stack } from "expo-router";
import React, { JSX } from "react";

export default function Layout(): JSX.Element {
  return (
    <Stack
      screenOptions={{
        headerShown: true,
        headerTitle: "Dorflix",
        headerTitleAlign: "center",
        headerTransparent: true,
        headerTitleStyle: { fontWeight: "700", color: "#fff", fontSize: 20 },
      }}
    >
      <Stack.Screen name="index" />
      <Stack.Screen name="video/[id]" />
    </Stack>
  );
}
