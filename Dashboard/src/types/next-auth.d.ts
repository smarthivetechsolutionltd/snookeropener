import "next-auth";

declare module "next-auth" {
  interface Session {
    user: {
      id?: string;
      name?: string | null;
      email?: string | null;
      image?: string | null;
      role?: string;
      companyName?: string;
      emailVerified?: boolean;
      needsOnboarding?: boolean;
    };
  }

  interface User {
    id?: string;
    role?: string;
    companyName?: string;
    emailVerified?: boolean;
    needsOnboarding?: boolean;
  }
}

