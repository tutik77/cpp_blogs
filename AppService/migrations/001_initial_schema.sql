CREATE TABLE IF NOT EXISTS users (
  id BIGSERIAL PRIMARY KEY,
  user_id BIGINT NOT NULL UNIQUE,
  username VARCHAR(255) NOT NULL,
  display_name VARCHAR(255),
  bio TEXT,
  avatar_path VARCHAR(512),
  created_at TIMESTAMP NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_user_id ON users(user_id);

CREATE TABLE IF NOT EXISTS follows (
  id BIGSERIAL PRIMARY KEY,
  follower_user_id BIGINT NOT NULL,
  following_user_id BIGINT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT now(),
  UNIQUE(follower_user_id, following_user_id)
);

CREATE INDEX IF NOT EXISTS idx_follower ON follows(follower_user_id);
CREATE INDEX IF NOT EXISTS idx_following ON follows(following_user_id);

CREATE TABLE IF NOT EXISTS posts (
  id BIGSERIAL PRIMARY KEY,
  author_user_id BIGINT NOT NULL,
  text TEXT NOT NULL,
  visibility VARCHAR(20) NOT NULL DEFAULT 'public',
  created_at TIMESTAMP NOT NULL DEFAULT now(),
  updated_at TIMESTAMP NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_author ON posts(author_user_id);
CREATE INDEX IF NOT EXISTS idx_created ON posts(created_at);

CREATE TABLE IF NOT EXISTS attachments (
  id BIGSERIAL PRIMARY KEY,
  post_id BIGINT NOT NULL,
  type VARCHAR(20) NOT NULL,
  file_path VARCHAR(512) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_post_id ON attachments(post_id);

CREATE TABLE IF NOT EXISTS likes (
  id BIGSERIAL PRIMARY KEY,
  post_id BIGINT NOT NULL,
  user_id BIGINT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT now(),
  UNIQUE(post_id, user_id)
);

CREATE INDEX IF NOT EXISTS idx_post_likes ON likes(post_id);

CREATE TABLE IF NOT EXISTS comments (
  id BIGSERIAL PRIMARY KEY,
  post_id BIGINT NOT NULL,
  author_user_id BIGINT NOT NULL,
  text TEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_post_comments ON comments(post_id);
CREATE INDEX IF NOT EXISTS idx_comment_created ON comments(created_at);
