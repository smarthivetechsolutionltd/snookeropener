export { default } from "next-auth/middleware";

export const config = {
  matcher: [
    /*
     * Match all request paths except for:
     * - auth (login, register)
     * - api/auth (NextAuth API, register API)
     * - api/snooker (Hardware device telemetry sync)
     * - _next/static, _next/image, favicon.ico, logo.png (static assets)
     */
    "/((?!auth|api/auth|api/snooker|_next/static|_next/image|favicon.ico|logo.png).*)",
  ],
};
