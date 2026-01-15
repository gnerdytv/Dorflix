import { Request, Response, NextFunction } from "express";
export interface AuthRequest extends Request {
    deviceId?: string;
    sessionId?: string;
}
export declare const generateSessionToken: (deviceId: string) => string;
export declare const verifySessionToken: (token: string) => Promise<{
    deviceId: string;
    sessionId: string;
} | null>;
export declare const authenticateDevice: (req: AuthRequest, res: Response, next: NextFunction) => Promise<Response<any, Record<string, any>> | undefined>;
export declare const optionalAuthenticateDevice: (req: AuthRequest, res: Response, next: NextFunction) => Promise<void>;
//# sourceMappingURL=auth.d.ts.map