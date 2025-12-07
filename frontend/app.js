// ==================== Configuration ====================
const CONFIG = {
    AUTH_API_URL: 'http://localhost:3000',
    APP_API_URL: 'http://localhost:3001'
};

// ==================== State Management ====================
const state = {
    user: null,
    token: null,
    currentPostId: null,
    posts: [],
    searchQuery: '',
    isSearchMode: false,
    avatarCrop: null
};

function saveAuth(token, identifier, options = {}) {
    const createProfile = options.createProfile === true;

    state.token = token;

    let displayName = identifier;
    const storedDisplayName = localStorage.getItem('display_name');

    if (!createProfile && storedDisplayName) {
        displayName = storedDisplayName;
    } else {
        localStorage.setItem('display_name', displayName);
    }

    state.user = { username: displayName };
    localStorage.setItem('token', token);
    localStorage.setItem('username', displayName);
    updateUI();

    if (createProfile) {
        (async () => {
            try {
                await apiCall(`${CONFIG.APP_API_URL}/users/me`, {
                    method: 'PUT',
                    body: JSON.stringify({ username: displayName })
                });
            } catch (e) {
                console.warn('Не удалось обновить профиль пользователя в AppService', e);
            }
        })();
    }
}

function loadAuth() {
    try {
        const token = localStorage.getItem('token');
        const displayName = localStorage.getItem('display_name') || localStorage.getItem('username');
        if (token && displayName) {
            state.token = token;
            state.user = { username: displayName };
            updateUI();
            loadFeed();
        }
    } catch (error) {
        console.error('Ошибка при загрузке авторизации:', error);
    }
}

function logout() {
    try {
        state.token = null;
        state.user = null;
        localStorage.removeItem('token');
        localStorage.removeItem('username');
        localStorage.removeItem('display_name');
        updateUI();
    } catch (error) {
        console.error('Ошибка при выходе из системы:', error);
    }
}

function updateUI() {
    const authButtons = document.getElementById('authButtons');
    const userMenu = document.getElementById('userMenu');
    const welcomeScreen = document.getElementById('welcomeScreen');
    const feedScreen = document.getElementById('feedScreen');
    const userName = document.getElementById('userName');

    if (state.user) {
        authButtons.style.display = 'none';
        userMenu.style.display = 'flex';
        welcomeScreen.style.display = 'none';
        feedScreen.style.display = 'block';
        userName.textContent = state.user.username;
    } else {
        authButtons.style.display = 'flex';
        userMenu.style.display = 'none';
        welcomeScreen.style.display = 'flex';
        feedScreen.style.display = 'none';
    }
}

// ==================== API Functions ====================
async function apiCall(url, options = {}) {
    const headers = {
        'Content-Type': 'application/json',
        ...options.headers
    };

    if (state.token && !options.skipAuth) {
        headers['Authorization'] = `Bearer ${state.token}`;
    }

    try {
        const response = await fetch(url, {
            ...options,
            headers
        });

        const data = await response.json().catch(() => ({}));

        if (!response.ok) {
            throw new Error(data.message || `HTTP error! status: ${response.status}`);
        }

        return data;
    } catch (error) {
        console.error('API Error:', error);
        throw error;
    }
}

// ==================== Auth API ====================
async function register(username, email, password) {
    try {
        const data = await apiCall(`${CONFIG.AUTH_API_URL}/v1/Auth/reg`, {
            method: 'POST',
            skipAuth: true,
            body: JSON.stringify({ name: username, login: email, password })
        });

        if (data.token) {
            saveAuth(data.token, username, { createProfile: true });
            closeModal('registerModal');
            showSuccess('Регистрация успешна!');
            loadFeed();
        }
        return data;
    } catch (error) {
        console.error('Ошибка регистрации:', error);
        throw error;
    }
}

async function login(username, password) {
    try {
        const data = await apiCall(`${CONFIG.AUTH_API_URL}/v1/Auth/login`, {
            method: 'POST',
            skipAuth: true,
            body: JSON.stringify({ login: username, password })
        });

        if (data.token) {
            saveAuth(data.token, username, { createProfile: false });
            closeModal('loginModal');
            showSuccess('Вход выполнен!');
            loadFeed();
        }
        return data;
    } catch (error) {
        console.error('Ошибка входа:', error);
        throw error;
    }
}

// ==================== Posts API ====================
async function loadFeed() {
    const feedContainer = document.getElementById('feedContainer');
    const feedLoading = document.getElementById('feedLoading');

    if (feedLoading) {
        feedLoading.style.display = 'flex';
    }

    try {
        const data = await apiCall(`${CONFIG.APP_API_URL}/feed`);
        state.posts = data.posts || [];
        state.isSearchMode = false;
        state.searchQuery = '';
        updateFeedSearchInfo();
        renderPosts(state.posts);
    } catch (error) {
        console.error('Error loading feed:', error);
        feedContainer.innerHTML = `
            <div class="empty-state">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"></circle>
                    <line x1="12" y1="8" x2="12" y2="12"></line>
                    <line x1="12" y1="16" x2="12.01" y2="16"></line>
                </svg>
                <h3>Ошибка загрузки ленты</h3>
                <p>${error.message}</p>
                <button class="btn btn-primary" onclick="loadFeed()">Попробовать снова</button>
            </div>
        `;
    } finally {
        if (feedLoading) {
            feedLoading.style.display = 'none';
        }
    }
}

async function createPost(title, content) {
    try {
        const data = await apiCall(`${CONFIG.APP_API_URL}/posts`, {
            method: 'POST',
            body: JSON.stringify({ title, content })
        });
        return data;
    } catch (error) {
        console.error('Ошибка создания поста:', error);
        throw error;
    }
}

async function deletePost(postId) {
    if (!confirm('Вы уверены, что хотите удалить этот пост?')) {
        return;
    }

    try {
        await apiCall(`${CONFIG.APP_API_URL}/posts/${postId}`, {
            method: 'DELETE'
        });

        showSuccess('Пост удален!');
        loadFeed();
    } catch (error) {
        showError('Ошибка при удалении поста');
        console.error('Error deleting post:', error);
    }
}

async function toggleLike(postId, isLiked) {
    try {
        const method = isLiked ? 'DELETE' : 'POST';
        await apiCall(`${CONFIG.APP_API_URL}/posts/${postId}/like`, { method });

        // Update local state
        const post = state.posts.find(p => p.id === postId);
        if (post) {
            post.is_liked = !isLiked;
            post.likes_count = isLiked ? post.likes_count - 1 : post.likes_count + 1;
            renderPosts(state.posts);
        }
    } catch (error) {
        console.error('Error toggling like:', error);
    }
}

async function uploadMediaFile(file) {
    try {
        const formData = new FormData();
        formData.append('file', file);

        const headers = {};
        if (state.token) {
            headers['Authorization'] = `Bearer ${state.token}`;
        }

        const response = await fetch(`${CONFIG.APP_API_URL}/media/upload`, {
            method: 'POST',
            headers,
            body: formData
        });

        const data = await response.json().catch(() => ({}));
        if (!response.ok) {
            throw new Error(data.error || `Upload failed: ${response.status}`);
        }
        return data;
    } catch (error) {
        console.error('Ошибка загрузки файла:', error);
        throw error;
    }
}

// ==================== Comments API ====================
async function loadComments(postId) {
    try {
        const data = await apiCall(`${CONFIG.APP_API_URL}/posts/${postId}/comments`);
        return data.comments || [];
    } catch (error) {
        console.error('Error loading comments:', error);
        return [];
    }
}

async function createComment(postId, content) {
    try {
        const data = await apiCall(`${CONFIG.APP_API_URL}/posts/${postId}/comments`, {
            method: 'POST',
            body: JSON.stringify({ content })
        });

        return data;
    } catch (error) {
        console.error('Ошибка создания комментария:', error);
        throw error;
    }
}

async function searchPostsByQuery(query) {
    try {
        const params = new URLSearchParams();
        params.set('q', query);
        const url = `${CONFIG.APP_API_URL}/posts/search?${params.toString()}`;
        const data = await apiCall(url);
        return data;
    } catch (error) {
        console.error('Ошибка поиска:', error);
        return { posts: [] };
    }
}

function openAvatarCrop(file) {
    try {
        const imgEl = document.getElementById('avatarCropImage');
        const zoomInput = document.getElementById('avatarCropZoom');
        const previewEl = document.getElementById('avatarCropPreview');
        if (!imgEl || !zoomInput || !previewEl) return;

        const reader = new FileReader();
        reader.onload = () => {
            const img = new Image();
            img.onload = () => {
                const rect = previewEl.getBoundingClientRect();
                const previewSize = Math.min(rect.width, rect.height) || 260;

                const scaleX = previewSize / img.width;
                const scaleY = previewSize / img.height;
                const baseScale = Math.max(scaleX, scaleY);

                state.avatarCrop = {
                    file,
                    image: img,
                    baseScale,
                    zoom: 1,
                    offsetX: 0,
                    offsetY: 0,
                    previewSize
                };
                imgEl.src = reader.result;
                zoomInput.value = '1';
                updateAvatarCropTransform();
                showModal('avatarCropModal');
            };
            img.src = reader.result;
        };
        reader.readAsDataURL(file);
    } catch (error) {
        console.error('Ошибка при открытии обрезки аватара:', error);
        showError('Ошибка при обработке изображения');
    }
}

function updateAvatarCropTransform() {
    try {
        const imgEl = document.getElementById('avatarCropImage');
        if (!imgEl || !state.avatarCrop) return;
        const { baseScale, zoom, offsetX, offsetY } = state.avatarCrop;
        const scale = baseScale * zoom;
        imgEl.style.transform = `translate(calc(-50% + ${offsetX}px), calc(-50% + ${offsetY}px)) scale(${scale})`;
    } catch (error) {
        console.error('Ошибка при обновлении трансформации:', error);
    }
}

function closeAvatarCropModal() {
    state.avatarCrop = null;
    closeModal('avatarCropModal');
}

function saveAvatarCrop() {
    try {
        if (!state.avatarCrop) return;
        const { image, baseScale, zoom, offsetX, offsetY, previewSize } = state.avatarCrop;
        const size = previewSize || 260;
        const canvas = document.createElement('canvas');
        canvas.width = size;
        canvas.height = size;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, size, size);

        const scale = baseScale * zoom;

        ctx.save();
        ctx.beginPath();
        ctx.arc(size / 2, size / 2, size / 2, 0, Math.PI * 2);
        ctx.closePath();
        ctx.clip();

        ctx.translate(size / 2 + offsetX, size / 2 + offsetY);
        ctx.scale(scale, scale);
        ctx.drawImage(image, -image.width / 2, -image.height / 2);

        ctx.restore();

        canvas.toBlob(async (blob) => {
            if (!blob) {
                showError('Не удалось подготовить аватар');
                return;
            }
            const file = new File([blob], 'avatar.png', { type: 'image/png' });
            try {
                const media = await uploadMediaFile(file);
                await apiCall(`${CONFIG.APP_API_URL}/users/me`, {
                    method: 'PUT',
                    body: JSON.stringify({ avatar_path: media.file_path })
                });
                closeAvatarCropModal();
                showMyProfile();
                showSuccess('Аватар обновлен');
            } catch (e) {
                showError('Не удалось обновить аватар');
                console.error(e);
            }
        }, 'image/png');
    } catch (error) {
        console.error('Ошибка при сохранении аватара:', error);
        showError('Ошибка при сохранении аватара');
    }
}

let avatarCropDragging = false;
let avatarCropStartX = 0;
let avatarCropStartY = 0;
let avatarCropStartOffsetX = 0;
let avatarCropStartOffsetY = 0;

function avatarCropPointerDown(clientX, clientY) {
    try {
        if (!state.avatarCrop) return;
        avatarCropDragging = true;
        avatarCropStartX = clientX;
        avatarCropStartY = clientY;
        avatarCropStartOffsetX = state.avatarCrop.offsetX;
        avatarCropStartOffsetY = state.avatarCrop.offsetY;
    } catch (error) {
        console.error('Ошибка при обработке указателя:', error);
    }
}

function avatarCropPointerMove(clientX, clientY) {
    try {
        if (!avatarCropDragging || !state.avatarCrop) return;
        const dx = clientX - avatarCropStartX;
        const dy = clientY - avatarCropStartY;
        state.avatarCrop.offsetX = avatarCropStartOffsetX + dx;
        state.avatarCrop.offsetY = avatarCropStartOffsetY + dy;
        clampAvatarCropOffsets();
        updateAvatarCropTransform();
    } catch (error) {
        console.error('Ошибка при перемещении указателя:', error);
    }
}

function avatarCropPointerUp() {
    avatarCropDragging = false;
}

function clampAvatarCropOffsets() {
    try {
        if (!state.avatarCrop) return;
        const { image, baseScale, zoom, previewSize } = state.avatarCrop;
        const scale = baseScale * zoom;
        const drawWidth = image.width * scale;
        const drawHeight = image.height * scale;
        const r = (previewSize || 260) / 2;
        const maxOffsetX = Math.max(0, drawWidth / 2 - r);
        const maxOffsetY = Math.max(0, drawHeight / 2 - r);
        state.avatarCrop.offsetX = Math.min(maxOffsetX, Math.max(-maxOffsetX, state.avatarCrop.offsetX));
        state.avatarCrop.offsetY = Math.min(maxOffsetY, Math.max(-maxOffsetY, state.avatarCrop.offsetY));
    } catch (error) {
        console.error('Ошибка при ограничении смещения:', error);
    }
}

// ==================== UI Functions ====================
function renderPosts(posts) {
    try {
        const feedContainer = document.getElementById('feedContainer');

        if (!posts || posts.length === 0) {
            feedContainer.innerHTML = `
                <div class="empty-state">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect>
                        <line x1="9" y1="9" x2="15" y2="9"></line>
                        <line x1="9" y1="15" x2="15" y2="15"></line>
                    </svg>
                    <h3>Здесь пока ничего нет</h3>
                    <p>Будьте первым, кто создаст пост!</p>
                    <button class="btn btn-primary" onclick="showCreatePost()">Создать пост</button>
                </div>
            `;
            return;
        }

        feedContainer.innerHTML = posts.map(post => createPostCard(post)).join('');
    } catch (error) {
        console.error('Ошибка при отрисовке постов:', error);
        const feedContainer = document.getElementById('feedContainer');
        if (feedContainer) {
            feedContainer.innerHTML = '<div class="error">Ошибка при загрузке постов</div>';
        }
    }
}

function updateFeedSearchInfo() {
    try {
        const info = document.getElementById('feedSearchInfo');
        if (!info) return;

        if (!state.isSearchMode || !state.searchQuery) {
            info.textContent = '';
            return;
        }

        const count = state.posts.length;
        if (!count) {
            info.textContent = `По запросу "${state.searchQuery}" ничего не найдено`;
        } else {
            info.textContent = `Найдено ${count} постов по запросу "${state.searchQuery}"`;
        }
    } catch (error) {
        console.error('Ошибка при обновлении информации поиска:', error);
    }
}

function createPostCard(post) {
    try {
        const currentUserId = state.token ? getUserIdFromToken(state.token) : null;
        const isOwner = currentUserId && post.author_user_id && currentUserId === post.author_user_id;
        const isLiked = post.is_liked || false;
        const likesCount = post.likes_count || 0;
        const commentsCount = post.comments_count || 0;

        // Backend currently хранит основной текст поста в поле "text".
        // Фронтенд может получать либо пару title/content, либо только text.
        const backendText = post.text || '';
        const content = post.content || backendText;
        // Если заголовка нет, берём первые символы текста как заголовок.
        const derivedTitle = backendText
            ? backendText.slice(0, 80) + (backendText.length > 80 ? '…' : '')
            : '';
        const title = post.title || derivedTitle;

        const authorInitial = (post.author_username || 'U')[0].toUpperCase();
        const postDate = post.created_at ? formatDate(post.created_at) : 'недавно';

        let avatarHtml = `<div class="author-avatar">${authorInitial}</div>`;
        if (post.author_avatar_path) {
            const avatarUrl = `${CONFIG.APP_API_URL}/${post.author_avatar_path}`;
            avatarHtml = `<div class="author-avatar author-avatar-image"><img src="${avatarUrl}" alt="${escapeHtml(post.author_username || '')}"></div>`;
        }

        let mediaHtml = '';
        if (post.attachments && post.attachments.length > 0) {
            const photo = post.attachments.find(a => a.type === 'photo');
            const video = post.attachments.find(a => a.type === 'video');

            if (photo) {
                const url = `${CONFIG.APP_API_URL}/${photo.file_path}`;
                mediaHtml = `<div class="post-media"><img src="${url}" alt=""></div>`;
            } else if (video) {
                const url = `${CONFIG.APP_API_URL}/${video.file_path}`;
                mediaHtml = `
                    <div class="post-media">
                        <video src="${url}" controls preload="metadata"></video>
                    </div>
                `;
            }
        }

        return `
            <div class="post-card">
                <div class="post-header">
                    <div class="post-author" onclick="showUserProfile(${post.author_user_id})">
                        ${avatarHtml}
                        <div class="author-info">
                            <h3>${escapeHtml(post.author_username || 'Аноним')}</h3>
                            <span class="post-date">${postDate}</span>
                        </div>
                    </div>
                    ${isOwner ? `
                        <div class="post-actions-header">
                            <button class="btn btn-ghost" onclick="deletePost(${post.id})" title="Удалить">
                                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                    <polyline points="3 6 5 6 21 6"></polyline>
                                    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                                </svg>
                                <span>Удалить</span>
                            </button>
                        </div>
                    ` : ''}
                </div>
                <h2 class="post-title">${escapeHtml(title)}</h2>
                <p class="post-content">${escapeHtml(content)}</p>
                ${mediaHtml}
                <div class="post-footer">
                    <button class="btn-icon ${isLiked ? 'liked' : ''}" onclick="toggleLike(${post.id}, ${isLiked})" title="Лайк">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="${isLiked ? 'currentColor' : 'none'}" stroke="currentColor" stroke-width="2">
                            <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"></path>
                        </svg>
                        <span class="post-stat">${likesCount}</span>
                    </button>
                    <button class="btn-icon" onclick="showComments(${post.id})" title="Комментарии">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
                        </svg>
                        <span class="post-stat">${commentsCount}</span>
                    </button>
                </div>
            </div>
        `;
    } catch (error) {
        console.error('Ошибка при создании карточки поста:', error);
        return '<div class="error">Ошибка при отображении поста</div>';
    }
}

function formatDate(dateString) {
    try {
        let date;
        try {
            if (!dateString) {
                return '';
            }
            if (dateString.endsWith('Z') || dateString.includes('T')) {
                date = new Date(dateString);
            } else {
                const iso = dateString.replace(' ', 'T') + 'Z';
                date = new Date(iso);
            }
            if (Number.isNaN(date.getTime())) {
                date = new Date(dateString);
            }
        } catch (e) {
            date = new Date(dateString);
        }

        const now = new Date();
        const diffMs = now - date;
        const diffMins = Math.floor(diffMs / 60000);
        const diffHours = Math.floor(diffMs / 3600000);
        const diffDays = Math.floor(diffMs / 86400000);

        if (diffMins < 1) return 'только что';
        if (diffMins < 60) return `${diffMins} мин. назад`;
        if (diffHours < 24) return `${diffHours} ч. назад`;
        if (diffDays < 7) return `${diffDays} дн. назад`;

        return date.toLocaleDateString('ru-RU', {
            day: 'numeric',
            month: 'short',
            year: date.getFullYear() !== now.getFullYear() ? 'numeric' : undefined
        });
    } catch (error) {
        console.error('Ошибка при форматировании даты:', error);
        return 'недавно';
    }
}

function escapeHtml(text) {
    try {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    } catch (error) {
        console.error('Ошибка при экранировании HTML:', error);
        return text || '';
    }
}

function getUserIdFromToken(token) {
    try {
        const parts = token.split('.');
        if (parts.length !== 3) return null;
        const payload = JSON.parse(atob(parts[1]));
        return parseInt(payload.user_id, 10);
    } catch (e) {
        return null;
    }
}

async function showUserProfile(userId) {
    try {
        if (!state.user) {
            showLogin();
            return;
        }

        const [user, posts, followers] = await Promise.all([
            apiCall(`${CONFIG.APP_API_URL}/users/${userId}`),
            apiCall(`${CONFIG.APP_API_URL}/users/${userId}/posts`),
            apiCall(`${CONFIG.APP_API_URL}/users/${userId}/followers`)
        ]);

        const currentId = state.token ? getUserIdFromToken(state.token) : null;
        const isSelf = currentId && user.user_id && currentId === user.user_id;
        const isFollowing = !isSelf && Array.isArray(followers)
            ? followers.some(f => f.user_id === currentId)
            : false;

        renderUserProfile(user, posts, { isSelf, isFollowing });
        showModal('profileModal');
    } catch (e) {
        showError('Не удалось загрузить профиль');
        console.error(e);
    }
}

function showMyProfile() {
    try {
        if (!state.token) {
            showLogin();
            return;
        }
        const userId = getUserIdFromToken(state.token);
        if (!userId) {
            showError('Не удалось определить пользователя');
            return;
        }
        showUserProfile(userId);
    } catch (error) {
        console.error('Ошибка при показе профиля:', error);
        showError('Ошибка при загрузке профиля');
    }
}

function renderUserProfile(user, posts, options = {}) {
    try {
        const profileTitle = document.getElementById('profileTitle');
        const profileAvatar = document.getElementById('profileAvatar');
        const profileUsername = document.getElementById('profileUsername');
        const profileDisplayName = document.getElementById('profileDisplayName');
        const profileBio = document.getElementById('profileBio');
        const profilePostsContainer = document.getElementById('profilePostsContainer');
        const profileActions = document.getElementById('profileActions');
        const profileFollowers = document.getElementById('profileFollowers');
        const profileFollowing = document.getElementById('profileFollowing');
        const profileFollowActions = document.getElementById('profileFollowActions');

        const username = user.username || '';
        const displayName = user.display_name || '';
        const bio = user.bio || '';
        const followersCount = user.followers_count || 0;
        const followingCount = user.following_count || 0;

        if (profileTitle) profileTitle.textContent = `Профиль ${username || displayName}`;
        if (profileUsername) profileUsername.textContent = username;
        if (profileDisplayName) profileDisplayName.textContent = displayName;
        if (profileBio) profileBio.textContent = bio;
        if (profileFollowers) profileFollowers.textContent = `Подписчики: ${followersCount}`;
        if (profileFollowing) profileFollowing.textContent = `Подписки: ${followingCount}`;

        if (profileAvatar) {
            if (user.avatar_path) {
                const url = `${CONFIG.APP_API_URL}/${user.avatar_path}`;
                profileAvatar.innerHTML = `<img src="${url}" alt="${escapeHtml(username || displayName || '')}">`;
            } else {
                const initial = (username || displayName || 'U')[0].toUpperCase();
                profileAvatar.textContent = initial;
            }
        }

        const currentId = state.token ? getUserIdFromToken(state.token) : null;
        const isSelf = options.isSelf !== undefined
            ? options.isSelf
            : currentId && user.user_id && currentId === user.user_id;
        const isFollowing = options.isFollowing === true;

        if (profileActions) {
            profileActions.style.display = isSelf ? 'block' : 'none';
        }

        if (profileFollowActions) {
            if (!isSelf && currentId) {
                profileFollowActions.style.display = 'block';
                profileFollowActions.innerHTML = `
                    <button class="btn btn-primary" onclick="${isFollowing ? `unfollowUser(${user.user_id})` : `followUser(${user.user_id})`}">
                        ${isFollowing ? 'Отписаться' : 'Подписаться'}
                    </button>
                `;
            } else {
                profileFollowActions.style.display = 'none';
                profileFollowActions.innerHTML = '';
            }
        }

        if (profilePostsContainer) {
            if (!posts || posts.length === 0) {
                profilePostsContainer.innerHTML = '<p>Постов пока нет</p>';
            } else {
                profilePostsContainer.innerHTML = posts.map(post => createPostCard(post)).join('');
            }
        }
    } catch (error) {
        console.error('Ошибка при отрисовке профиля:', error);
        showError('Ошибка при отображении профиля');
    }
}

async function updateAvatar() {
    try {
        const input = document.getElementById('avatarFile');
        if (!input || !input.files || !input.files[0]) return;

        openAvatarCrop(input.files[0]);
    } catch (error) {
        console.error('Ошибка при обновлении аватара:', error);
        showError('Ошибка при выборе аватара');
    }
}

// ==================== Modal Functions ====================
function showModal(modalId) {
    try {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.classList.add('show');
            document.body.style.overflow = 'hidden';
        }
    } catch (error) {
        console.error('Ошибка при показе модального окна:', error);
    }
}

function closeModal(modalId) {
    try {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.classList.remove('show');
            document.body.style.overflow = '';

            // Clear form if exists
            const form = modal.querySelector('form');
            if (form) {
                form.reset();
                const errorDiv = form.querySelector('.form-error');
                if (errorDiv) {
                    errorDiv.classList.remove('show');
                    errorDiv.textContent = '';
                }
            }
        }
    } catch (error) {
        console.error('Ошибка при закрытии модального окна:', error);
    }
}

function showLogin() {
    showModal('loginModal');
}

function showRegister() {
    showModal('registerModal');
}

function showCreatePost() {
    if (!state.user) {
        showLogin();
        return;
    }
    showModal('createPostModal');
}

async function showComments(postId) {
    try {
        if (!state.user) {
            showLogin();
            return;
        }

        state.currentPostId = postId;
        showModal('commentsModal');

        const commentsContainer = document.getElementById('commentsContainer');
        commentsContainer.innerHTML = '<div class="loading"><div class="spinner"></div></div>';

        try {
            const comments = await loadComments(postId);
            renderComments(comments);
        } catch (error) {
            commentsContainer.innerHTML = `
                <div class="empty-state">
                    <p>Ошибка загрузки комментариев</p>
                </div>
            `;
        }
    } catch (error) {
        console.error('Ошибка при показе комментариев:', error);
        showError('Ошибка при загрузке комментариев');
    }
}

function renderComments(comments) {
    try {
        const commentsContainer = document.getElementById('commentsContainer');

        if (!comments || comments.length === 0) {
            commentsContainer.innerHTML = `
                <div class="empty-state">
                    <p>Комментариев пока нет. Будьте первым!</p>
                </div>
            `;
            return;
        }

        commentsContainer.innerHTML = comments.map(comment => {
            const content = comment.content || comment.text || '';
            return `
            <div class="comment-item">
                <div class="comment-header">
                    <span class="comment-author">${escapeHtml(comment.author_username || 'Аноним')}</span>
                    <span class="comment-date">${formatDate(comment.created_at)}</span>
                </div>
                <div class="comment-content">${escapeHtml(content)}</div>
            </div>
        `;
        }).join('');
    } catch (error) {
        console.error('Ошибка при отрисовке комментариев:', error);
        const commentsContainer = document.getElementById('commentsContainer');
        if (commentsContainer) {
            commentsContainer.innerHTML = '<div class="error">Ошибка при загрузке комментариев</div>';
        }
    }
}

// ==================== Form Handlers ====================
function handleFormError(formId, errorId, error) {
    try {
        const errorDiv = document.getElementById(errorId);
        if (errorDiv) {
            errorDiv.textContent = error.message || 'Произошла ошибка';
            errorDiv.classList.add('show');
        }
    } catch (error) {
        console.error('Ошибка при обработке ошибки формы:', error);
    }
}

function showSuccess(message) {
    try {
        showToast(message, 'success');
    } catch (error) {
        console.error('Ошибка при показе успеха:', error);
    }
}

function showError(message) {
    try {
        showToast(message, 'error');
    } catch (error) {
        console.error('Ошибка при показе ошибки:', error);
        alert(message); // Fallback
    }
}

function showToast(message, type = 'success') {
    try {
        let container = document.getElementById('toastContainer');
        if (!container) {
            container = document.createElement('div');
            container.id = 'toastContainer';
            container.className = 'toast-container';
            document.body.appendChild(container);
        }

        const toast = document.createElement('div');
        toast.className = 'toast ' + (type === 'error' ? 'toast-error' : 'toast-success');
        toast.textContent = message;
        container.appendChild(toast);

        requestAnimationFrame(() => {
            toast.classList.add('show');
        });

        setTimeout(() => {
            toast.classList.remove('show');
            setTimeout(() => {
                toast.remove();
                if (!container.children.length) {
                    container.remove();
                }
            }, 300);
        }, 3000);
    } catch (error) {
        console.error('Ошибка при показе тоста:', error);
        alert(message); // Fallback
    }
}

async function followUser(userId) {
    try {
        await apiCall(`${CONFIG.APP_API_URL}/users/${userId}/follow`, {
            method: 'POST'
        });
        showSuccess('Вы подписались на пользователя');
        await showUserProfile(userId);
    } catch (e) {
        showError('Не удалось подписаться');
        console.error(e);
    }
}

async function unfollowUser(userId) {
    try {
        await apiCall(`${CONFIG.APP_API_URL}/users/${userId}/follow`, {
            method: 'DELETE'
        });
        showSuccess('Вы отписались от пользователя');
        await showUserProfile(userId);
    } catch (e) {
        showError('Не удалось отписаться');
        console.error(e);
    }
}

// ==================== Event Listeners ====================
document.addEventListener('DOMContentLoaded', () => {
    try {
        // Load auth state
        loadAuth();

        // Login form
        const loginForm = document.getElementById('loginForm');
        if (loginForm) {
            loginForm.addEventListener('submit', async (e) => {
                e.preventDefault();
                const username = document.getElementById('loginUsername').value;
                const password = document.getElementById('loginPassword').value;

                try {
                    await login(username, password);
                } catch (error) {
                    handleFormError('loginForm', 'loginError', error);
                }
            });
        }

        // Register form
        const registerForm = document.getElementById('registerForm');
        if (registerForm) {
            registerForm.addEventListener('submit', async (e) => {
                e.preventDefault();
                const username = document.getElementById('registerUsername').value;
                const email = document.getElementById('registerEmail').value;
                const password = document.getElementById('registerPassword').value;

                try {
                    await register(username, email, password);
                } catch (error) {
                    handleFormError('registerForm', 'registerError', error);
                }
            });
        }

        // Create post form
        const createPostForm = document.getElementById('createPostForm');
        if (createPostForm) {
            createPostForm.addEventListener('submit', async (e) => {
                e.preventDefault();
                const title = document.getElementById('postTitle').value;
                const content = document.getElementById('postContent').value;
                const mediaInput = document.getElementById('postMedia');
                const file = mediaInput && mediaInput.files && mediaInput.files[0] ? mediaInput.files[0] : null;

                try {
                    const post = await createPost(title, content);

                    if (file && post && post.id) {
                        try {
                            const media = await uploadMediaFile(file);
                            await apiCall(`${CONFIG.APP_API_URL}/posts/${post.id}/attach`, {
                                method: 'POST',
                                body: JSON.stringify({ file_path: media.file_path, type: media.type })
                            });
                        } catch (mediaError) {
                            console.warn('Не удалось прикрепить медиа:', mediaError);
                        }
                    }

                    closeModal('createPostModal');
                    showSuccess('Пост успешно создан!');
                    loadFeed();
                    createPostForm.reset();
                } catch (error) {
                    handleFormError('createPostForm', 'createPostError', error);
                }
            });
        }

        const feedSearchInput = document.getElementById('feedSearchInput');
        const feedSearchButton = document.getElementById('feedSearchButton');

        async function handleSearch() {
            try {
                if (!feedSearchInput) return;
                const query = feedSearchInput.value.trim();
                if (!query) {
                    state.isSearchMode = false;
                    state.searchQuery = '';
                    updateFeedSearchInfo();
                    loadFeed();
                    return;
                }

                try {
                    const data = await searchPostsByQuery(query);
                    state.posts = data.posts || [];
                    state.isSearchMode = true;
                    state.searchQuery = query;
                    renderPosts(state.posts);
                    updateFeedSearchInfo();
                } catch (error) {
                    showError('Ошибка при поиске постов');
                    console.error('Error searching posts:', error);
                }
            } catch (error) {
                console.error('Ошибка при обработке поиска:', error);
            }
        }

        if (feedSearchButton && feedSearchInput) {
            feedSearchButton.addEventListener('click', (e) => {
                e.preventDefault();
                handleSearch();
            });

            feedSearchInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    handleSearch();
                } else if (e.key === 'Escape') {
                    feedSearchInput.value = '';
                    state.isSearchMode = false;
                    state.searchQuery = '';
                    updateFeedSearchInfo();
                    loadFeed();
                }
            });
        }

        // Add comment form
        const addCommentForm = document.getElementById('addCommentForm');
        if (addCommentForm) {
            addCommentForm.addEventListener('submit', async (e) => {
                e.preventDefault();
                const content = document.getElementById('commentContent').value;

                if (!state.currentPostId) return;

                try {
                    await createComment(state.currentPostId, content);
                    addCommentForm.reset();

                    // Reload comments
                    const comments = await loadComments(state.currentPostId);
                    renderComments(comments);

                    // Update comment count in feed
                    loadFeed();
                } catch (error) {
                    showError('Ошибка при добавлении комментария');
                    console.error('Error adding comment:', error);
                }
            });
        }

        const avatarCropPreview = document.getElementById('avatarCropPreview');
        const avatarCropZoom = document.getElementById('avatarCropZoom');

        if (avatarCropPreview) {
            avatarCropPreview.addEventListener('mousedown', (e) => {
                e.preventDefault();
                avatarCropPointerDown(e.clientX, e.clientY);
            });
            avatarCropPreview.addEventListener('wheel', (e) => {
                if (!state.avatarCrop) return;
                e.preventDefault();
                const zoomInputEl = document.getElementById('avatarCropZoom');
                let nextZoom = state.avatarCrop.zoom || 1;
                const delta = e.deltaY > 0 ? -0.1 : 0.1;
                nextZoom = Math.min(3, Math.max(1, nextZoom + delta));
                state.avatarCrop.zoom = nextZoom;
                if (zoomInputEl) {
                    zoomInputEl.value = String(nextZoom);
                }
                clampAvatarCropOffsets();
                updateAvatarCropTransform();
            }, { passive: false });
            window.addEventListener('mousemove', (e) => {
                if (avatarCropDragging) {
                    e.preventDefault();
                    avatarCropPointerMove(e.clientX, e.clientY);
                }
            });
            window.addEventListener('mouseup', () => {
                avatarCropPointerUp();
            });

            avatarCropPreview.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (!touch) return;
                avatarCropPointerDown(touch.clientX, touch.clientY);
            }, { passive: true });
            window.addEventListener('touchmove', (e) => {
                if (!avatarCropDragging) return;
                const touch = e.touches[0];
                if (!touch) return;
                avatarCropPointerMove(touch.clientX, touch.clientY);
            }, { passive: true });
            window.addEventListener('touchend', () => {
                avatarCropPointerUp();
            });
        }

        if (avatarCropZoom) {
            avatarCropZoom.addEventListener('input', (e) => {
                if (!state.avatarCrop) return;
                const value = parseFloat(e.target.value);
                if (!Number.isNaN(value)) {
                    state.avatarCrop.zoom = value;
                    clampAvatarCropOffsets();
                    updateAvatarCropTransform();
                }
            });
        }

        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                const openModal = document.querySelector('.modal.show');
                if (openModal) {
                    if (openModal.id === 'avatarCropModal') {
                        closeAvatarCropModal();
                    } else {
                        closeModal(openModal.id);
                    }
                }
            }
        });
    } catch (error) {
        console.error('Ошибка при инициализации приложения:', error);
        alert('Ошибка при загрузке приложения. Пожалуйста, перезагрузите страницу.');
    }
});