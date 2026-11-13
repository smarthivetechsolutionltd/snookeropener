import type { Metadata } from "next";
import "./globals.css";
import { Providers } from "@/components/Providers";

export const metadata: Metadata = {
  title: "Smarthive Pool | Hardware-Enabled SaaS (H-SaaS) Platform",
  description: "Enterprise Billiards Fleet & Automated Revenue Telematics Management System",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    // suppressHydrationWarning: browser extensions inject attributes into <html>/<body>
    <html lang="en" suppressHydrationWarning>
      <body
        suppressHydrationWarning
        className="bg-slate-950 text-slate-100 antialiased selection:bg-emerald-500 selection:text-white"
      >
        <Providers>{children}</Providers>
      </body>
    </html>
  );
}
