import { Pool } from "pg";
declare const pool: Pool;
export declare const testConnection: () => Promise<boolean>;
export declare const connectDatabase: () => Promise<void>;
export declare const query: (text: string, params?: any[]) => Promise<import("pg").QueryResult<any>>;
export interface Device {
    id: string;
    device_id: string;
    device_name?: string;
    device_type?: string;
    device_model?: string;
    os_version?: string;
    app_version?: string;
    created_at: Date;
    updated_at: Date;
}
export interface DeviceSession {
    id: string;
    device_id: string;
    session_token: string;
    expires_at: Date;
    is_active: boolean;
    created_at: Date;
    updated_at: Date;
}
export interface Video {
    id: string;
    title: string;
    description?: string;
    video_url: string;
    thumbnail_url?: string;
    duration: number;
    uploader_name: string;
    uploader_avatar_url?: string;
    likes_count: number;
    comments_count: number;
    shares_count: number;
    views_count: number;
    is_active: boolean;
    created_at: Date;
    updated_at: Date;
}
export interface WatchProgress {
    id: string;
    video_id: string;
    device_id: string;
    progress_seconds: number;
    total_duration: number;
    percentage_watched: number;
    is_completed: boolean;
    last_watched_at: Date;
    created_at: Date;
    updated_at: Date;
}
export interface VideoView {
    id: string;
    video_id: string;
    device_id: string;
    view_duration: number;
    is_qualified_view: boolean;
    created_at: Date;
}
export interface VideoLike {
    id: string;
    video_id: string;
    device_id: string;
    created_at: Date;
}
export interface VideoComment {
    id: string;
    video_id: string;
    device_id: string;
    comment_text: string;
    is_active: boolean;
    created_at: Date;
    updated_at: Date;
}
export interface VideoShare {
    id: string;
    video_id: string;
    device_id: string;
    share_platform?: string;
    created_at: Date;
}
export default pool;
//# sourceMappingURL=db.d.ts.map