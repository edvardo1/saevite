#if !defined(GOOEY_H)
#define GOOEY_H

#if __STDC_VERSION__ >= 199901L
#define inline static inline
#else
#define inline
#endif

typedef enum {
	gooey_WindowFlag_Fullscreen = SDL_WINDOW_FULLSCREEN,
	gooey_WindowFlag_OpenGL = SDL_WINDOW_OPENGL,
	gooey_WindowFlag_Occluded = SDL_WINDOW_OCCLUDED,
	gooey_WindowFlag_Hidden = SDL_WINDOW_HIDDEN,
	gooey_WindowFlag_Borderless = SDL_WINDOW_BORDERLESS,
	gooey_WindowFlag_Resizable = SDL_WINDOW_RESIZABLE,
	gooey_WindowFlag_Minimized = SDL_WINDOW_MINIMIZED,
	gooey_WindowFlag_Maximized = SDL_WINDOW_MAXIMIZED,
	gooey_WindowFlag_MouseGrabbed = SDL_WINDOW_MOUSE_GRABBED,
	gooey_WindowFlag_InputFocus = SDL_WINDOW_INPUT_FOCUS,
	gooey_WindowFlag_MouseFocus = SDL_WINDOW_MOUSE_FOCUS,
	gooey_WindowFlag_External = SDL_WINDOW_EXTERNAL,
	gooey_WindowFlag_Modal = SDL_WINDOW_MODAL,
	gooey_WindowFlag_HighPixelDensity = SDL_WINDOW_HIGH_PIXEL_DENSITY,
	gooey_WindowFlag_MouseCapture = SDL_WINDOW_MOUSE_CAPTURE,
	gooey_WindowFlag_AlwaysOnTop = SDL_WINDOW_ALWAYS_ON_TOP,
	gooey_WindowFlag_Utility = SDL_WINDOW_UTILITY,
	gooey_WindowFlag_Tooltip = SDL_WINDOW_TOOLTIP,
	gooey_WindowFlag_PopupMenu = SDL_WINDOW_POPUP_MENU,
	gooey_WindowFlag_KeyboardGrabbed = SDL_WINDOW_KEYBOARD_GRABBED,
	gooey_WindowFlag_Vulkan = SDL_WINDOW_VULKAN,
	gooey_WindowFlag_Metal = SDL_WINDOW_METAL,
	gooey_WindowFlag_Transparent = SDL_WINDOW_TRANSPARENT,
	gooey_WindowFlag_NotFocusable = SDL_WINDOW_NOT_FOCUSABLE
} gooey_WindowFlag;

#define gooey_KEYMOD_LSHIFT  (1 << 0x00) /* 0x0001 */
#define gooey_KEYMOD_RSHIFT  (1 << 0x01) /* 0x0002 */
#define gooey_KEYMOD_LEVEL5  (1 << 0x02) /* 0x0004 */
#define gooey_KEYMOD_LCTRL   (1 << 0x06) /* 0x0040 */
#define gooey_KEYMOD_RCTRL   (1 << 0x07) /* 0x0080 */
#define gooey_KEYMOD_LALT    (1 << 0x08) /* 0x0100 */
#define gooey_KEYMOD_RALT    (1 << 0x09) /* 0x0200 */
#define gooey_KEYMOD_LGUI    (1 << 0x0a) /* 0x0480 */
#define gooey_KEYMOD_RGUI    (1 << 0x0b) /* 0x0800 */
#define gooey_KEYMOD_NUM     (1 << 0x0c) /* 0x1000 */
#define gooey_KEYMOD_CAPS    (1 << 0x0d) /* 0x2000 */
#define gooey_KEYMOD_MODE    (1 << 0x0e) /* 0x4800 */
#define gooey_KEYMOD_SCROLL  (1 << 0x0f) /* 0x8000 */

typedef struct {
	char *name;
	int w;
	int h;
	gooey_WindowFlag windowFlags;
} gooey_Cfg;

typedef struct {
	SDL_Event ev;
} gooey_Event;

typedef struct {
	SDL_Texture *texture;
} gooey_Texture;

#define EVENT_QUEUE_LEN 16
typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	bool shouldClose;

	gooey_Event event_queue[EVENT_QUEUE_LEN];
	int event_queue_len;

	Rect_I32 renderRegion;

	/* @todo implement allocator */
	void (*allocator)(void *, size_t);
	void (*free)(void *, void *);
	void *closure_data;

	/* @todo implement arena */
	U8 *arena;
	size_t arena_size;
	size_t arena_count;
} gooey_Ctx;

int gooey_init(gooey_Ctx *ctx, const gooey_Cfg *cfg);
void gooey_exit(gooey_Ctx *ctx);
void gooey_setWindowShouldClose(gooey_Ctx *ctx, bool what);
bool gooey_windowShouldClose(gooey_Ctx *ctx);

inline bool gooey_event_isMouseWheel(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_mouse_isDown(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_mouse_isUp(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_mouse_isMotion(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_mouseWheel_getXScrolled(
	gooey_Ctx *ctx, const gooey_Event *ev, float *x
);
inline bool gooey_event_mouseWheel_getYScrolled(
	gooey_Ctx *ctx, const gooey_Event *ev, float *y
);
inline bool gooey_event_mouse_getX(
	gooey_Ctx *ctx, const gooey_Event *ev, I32 *x
);
inline bool gooey_event_mouse_getY(
	gooey_Ctx *ctx, const gooey_Event *ev, I32 *y
);
inline bool gooey_event_mouse_isLeft(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_mouse_isRight(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_key_isDown(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_key_isUp(
	gooey_Ctx *ctx, const gooey_Event *ev
);
inline bool gooey_event_key_getKey(
	gooey_Ctx *ctx, const gooey_Event *ev, U64 *key
);
inline void gooey_event_addMouseOffset(
	gooey_Ctx *ctx, gooey_Event *ev, int x, int y
);

inline void gooey_setRenderRegion(gooey_Ctx *ctx, Rect_I32 rect);
inline Rect_I32 gooey_getRenderRegion(gooey_Ctx *ctx);

inline bool gooey_pollEvent(gooey_Ctx *ctx, gooey_Event *gev);
inline bool gooey_waitEvent(gooey_Ctx *ctx, gooey_Event *gev);

U64 gooey_getNs(gooey_Ctx *ctx);
void gooey_delayNs(gooey_Ctx *ctx, U64 ns);

void gooey_clear(gooey_Ctx *ctx);
void gooey_present(gooey_Ctx *ctx);
void gooey_setDrawColor(gooey_Ctx *ctx, U32 color);

/* @todo implement draw and fill */
inline void gooey_fillRect(gooey_Ctx *ctx, Rect_I32 rect);
inline void gooey_fillPolygon(
	gooey_Ctx *ctx, Vec2_I32 *vertices, size_t vertices_amount
);
inline void gooey_drawRect(gooey_Ctx *ctx, Rect_I32 rect);
inline void gooey_drawPolygon(
	gooey_Ctx *ctx, Vec2_I32 *vertices, size_t vertices_amount
);

inline int gooey_loadTexture(
	gooey_Ctx *ctx,
	gooey_Texture *out_texture,
	const U32 *pixels,
	size_t width,
	size_t height
);
inline int gooey_renderTexture(
	gooey_Ctx *ctx, gooey_Texture texture, Rect_I32 dst, Rect_I32 src
);
inline void gooey_setRenderTarget(gooey_Ctx *ctx, gooey_Texture texture);
inline void gooey_setRenderTargetToRoot(gooey_Ctx *ctx);
inline int gooey_destroyTexture(gooey_Ctx *ctx, gooey_Texture texture);

#if defined(GOOEY_IMPLEMENTATION)

int gooey_init(gooey_Ctx *ctx, const gooey_Cfg *cfg) {
	gooey_Cfg standard_cfg = {0};

	int success = SDL_Init(SDL_INIT_VIDEO);
	if (!success) {
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	if (cfg == NULL) {
		cfg = &standard_cfg;
		standard_cfg.w = 800;
		standard_cfg.h = 600;
		standard_cfg.windowFlags = 0;
		standard_cfg.name = "standard window name";
	}
	
	ctx->window = SDL_CreateWindow(
		cfg->name, cfg->w, cfg->h, cfg->windowFlags
	);
	if (ctx->window == NULL) {
		fprintf(stderr, "SDL_GetError: %s\n", SDL_GetError());
		return 1;
	}

	ctx->renderer = SDL_CreateRenderer(ctx->window, NULL);
	if (ctx->renderer == NULL) {
		fprintf(stderr, "SDL_GetError: %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

	ctx->renderRegion = make_Rect_I32(0, 0, cfg->w, cfg->h);

	return 0;
}

void gooey_exit(gooey_Ctx *ctx) {
	UNUSED(ctx);
	SDL_Quit();
}

inline bool gooey_event_isMouseWheel(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_MOUSE_WHEEL;
}

inline bool gooey_event_mouse_isDown(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

inline bool gooey_event_mouse_isUp(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_MOUSE_BUTTON_UP;
}

inline bool gooey_event_mouse_isMotion(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_MOUSE_MOTION;
}

inline bool gooey_event_mouseWheel_getXScrolled(
	gooey_Ctx *ctx, const gooey_Event *ev, float *x
) {
	const bool ismw = gooey_event_isMouseWheel(ctx, ev);
	if (ismw) {
		*x = ev->ev.wheel.x;
		return 0;
	} else {
		return 1;
	}
}

inline bool gooey_event_mouseWheel_getYScrolled(
	gooey_Ctx *ctx, const gooey_Event *ev, float *y
) {
	const bool ismw = gooey_event_isMouseWheel(ctx, ev);
	if (ismw) {
		*y = ev->ev.wheel.y;
		return 0;
	} else {
		return 1;
	}
}

inline bool gooey_event_mouse_getX(
	gooey_Ctx *ctx, const gooey_Event *ev, I32 *x
) {
	const bool isMotion = gooey_event_mouse_isMotion(ctx, ev);
	const bool isUp = gooey_event_mouse_isUp(ctx, ev);
	const bool isDown = gooey_event_mouse_isDown(ctx, ev);
	if (isMotion) {
		*x =  ev->ev.motion.x;
		return 0;
	} else if(isUp || isDown) {
		*x = ev->ev.button.x;
		return 0;
	} else {
		return 1;
	}
}

inline bool gooey_event_mouse_getY(
	gooey_Ctx *ctx, const gooey_Event *ev, I32 *y
) {
	const bool isMotion = gooey_event_mouse_isMotion(ctx, ev);
	const bool isUp = gooey_event_mouse_isUp(ctx, ev);
	const bool isDown = gooey_event_mouse_isDown(ctx, ev);
	if (isMotion) {
		*y = ev->ev.motion.y;
		return 0;
	} else if(isUp || isDown) {
		*y = ev->ev.button.y;
		return 0;
	} else {
		return 1;
	}
}

inline bool gooey_event_mouse_isLeft(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	const bool isMotion = gooey_event_mouse_isMotion(ctx, ev);
	const bool isUp = gooey_event_mouse_isUp(ctx, ev);
	const bool isDown = gooey_event_mouse_isDown(ctx, ev);
	if (isMotion) {
		return !!(ev->ev.motion.state & SDL_BUTTON_LEFT);
	} else if(isUp || isDown) {
		return !!(ev->ev.button.button & SDL_BUTTON_LEFT);
	} else {
		return false;
	}
}

inline bool gooey_event_mouse_isRight(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	const bool isMotion = gooey_event_mouse_isMotion(ctx, ev);
	const bool isUp = gooey_event_mouse_isUp(ctx, ev);
	const bool isDown = gooey_event_mouse_isDown(ctx, ev);
	if (isMotion) {
		return !!(ev->ev.motion.state & SDL_BUTTON_RIGHT);
	} else if(isUp || isDown) {
		return !!(ev->ev.button.button & SDL_BUTTON_RIGHT);
	} else {
		return false;
	}
}

inline bool gooey_event_key_isDown(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_KEY_DOWN;
}

inline bool gooey_event_key_isUp(
	gooey_Ctx *ctx, const gooey_Event *ev
) {
	UNUSED(ctx);
	return ev->ev.type == SDL_EVENT_KEY_UP;
}

inline bool gooey_event_key_getKey(
	gooey_Ctx *ctx, const gooey_Event *ev, U64 *key
) {
	SDL_Keycode keycode = {0};
	if (gooey_event_key_isUp(ctx, ev) || gooey_event_key_isDown(ctx, ev)) {
		keycode = SDL_GetKeyFromScancode(ev->ev.key.scancode, ev->ev.key.mod, false);
		*key = keycode;
		return 0;
	} else {
		return 1;
	}
}

inline bool gooey_event_key_get(
	gooey_Ctx *ctx, const gooey_Event *ev, U64 *key, U32 *keyMod, Bool *isDown
) {
	UNUSED(ctx);

	SDL_Keycode keycode = {0};
	if (ev->ev.type == SDL_EVENT_KEY_UP || ev->ev.type == SDL_EVENT_KEY_DOWN) {
		if (isDown != NULL) {
			*isDown = ev->ev.type == SDL_EVENT_KEY_DOWN;
		}

		if (key != NULL) {
			keycode = SDL_GetKeyFromScancode(ev->ev.key.scancode, ev->ev.key.mod, false);
			//printf("name: %s\n", SDL_GetKeyName(keycode));
			*key = keycode;
		}

		if (keyMod != NULL) {
			*keyMod = 
				((!!(ev->ev.key.mod & SDL_KMOD_LSHIFT)) << 0x00) | /* 0x0001 */
				((!!(ev->ev.key.mod & SDL_KMOD_RSHIFT)) << 0x01) | /* 0x0002 */
				((!!(ev->ev.key.mod & SDL_KMOD_LEVEL5)) << 0x02) | /* 0x0004 */
				((!!(ev->ev.key.mod & SDL_KMOD_LCTRL))  << 0x06) | /* 0x0040 */
				((!!(ev->ev.key.mod & SDL_KMOD_RCTRL))  << 0x07) | /* 0x0080 */
				((!!(ev->ev.key.mod & SDL_KMOD_LALT))   << 0x08) | /* 0x0100 */
				((!!(ev->ev.key.mod & SDL_KMOD_RALT))   << 0x09) | /* 0x0200 */
				((!!(ev->ev.key.mod & SDL_KMOD_LGUI))   << 0x0a) | /* 0x0480 */
				((!!(ev->ev.key.mod & SDL_KMOD_RGUI))   << 0x0b) | /* 0x0800 */
				((!!(ev->ev.key.mod & SDL_KMOD_NUM))    << 0x0c) | /* 0x1000 */
				((!!(ev->ev.key.mod & SDL_KMOD_CAPS))   << 0x0d) | /* 0x2000 */
				((!!(ev->ev.key.mod & SDL_KMOD_MODE))   << 0x0e) | /* 0x4800 */
				((!!(ev->ev.key.mod & SDL_KMOD_SCROLL)) << 0x0f);  /* 0x8000 */

			/*
			 * since gooey is based on SDL
			 * we should make sure the mod keys are compatible with it
			 */
			assert(*keyMod == ev->ev.key.mod);
		}

		return 0;
	} else {
		return 1;
	}
}

inline void gooey_event_addMouseOffset(
	gooey_Ctx *ctx, gooey_Event *ev, int x, int y
) {
	const bool isMotion = gooey_event_mouse_isMotion(ctx, ev);
	const bool isUp = gooey_event_mouse_isUp(ctx, ev);
	const bool isDown = gooey_event_mouse_isDown(ctx, ev);
	if (isMotion) {
		ev->ev.motion.x += x;
		ev->ev.motion.y += y;
	} else if(isUp || isDown) {
		ev->ev.button.x += x;
		ev->ev.button.y += y;
	}
}

inline void gooey_setRenderRegion(gooey_Ctx *ctx, Rect_I32 rect) {
	ctx->renderRegion = rect;
}

inline Rect_I32 gooey_getRenderRegion(gooey_Ctx *ctx) {
	return ctx->renderRegion;
}

inline bool gooey_pollEvent(gooey_Ctx *ctx, gooey_Event *gev) {
	bool ret = false;

	if (ctx->event_queue_len > 0) {
		ctx->event_queue_len -= 1;
		*gev = ctx->event_queue[ctx->event_queue_len];
		return true;
	}

	ret = SDL_PollEvent(&gev->ev);

	if (gev->ev.type == SDL_EVENT_QUIT) {
		ctx->shouldClose = true;
	}

	return ret;
}

inline bool gooey_waitEvent(gooey_Ctx *ctx, gooey_Event *gev) {
	bool ret = false;
	if (ctx->event_queue_len > 0) {
		ctx->event_queue_len -= 1;
		*gev = ctx->event_queue[ctx->event_queue_len];
		return true;
	}

	ret = SDL_WaitEvent(&gev->ev);

	if (gev->ev.type == SDL_EVENT_QUIT) {
		ctx->shouldClose = true;
	}

	return ret;
}

void gooey_setWindowShouldClose(gooey_Ctx *ctx, bool what) {
	ctx->shouldClose = what;
}

bool gooey_windowShouldClose(gooey_Ctx *ctx) {
	if (ctx->shouldClose) {
		return true;
	}

	/*gooey_Event gev;
	if (SDL_PollEvent(&gev.ev)) {
		if (gev.ev.type == SDL_EVENT_QUIT) {
			ctx->shouldClose = true;
			return true;
		} else {
			ctx->event_queue[ctx->event_queue_len] = gev;
			ctx->event_queue_len =
				(ctx->event_queue_len + 1) % EVENT_QUEUE_LEN;
		}
	}*/

	return false;
}

U64 gooey_getNs(gooey_Ctx *ctx) {
	UNUSED(ctx);
	return SDL_GetTicksNS();
}

void gooey_delayNs(gooey_Ctx *ctx, U64 ns) {
	UNUSED(ctx);
	SDL_DelayNS(ns);
}

void gooey_clear(gooey_Ctx *ctx) {
	SDL_FRect fr;
	fr.x = ctx->renderRegion.x;
	fr.y = ctx->renderRegion.y;
	fr.w = ctx->renderRegion.w;
	fr.h = ctx->renderRegion.h;
	SDL_RenderFillRect(ctx->renderer, &fr);
}

void gooey_present(gooey_Ctx *ctx) {
	bool success = SDL_RenderPresent(ctx->renderer);
	if (!success) {
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		fflush(stderr);
		abort();
	}
}

void gooey_setDrawColor(gooey_Ctx *ctx, U32 color) {
	const U32 r = (color >> 24) & 0xff;
	const U32 g = (color >> 16) & 0xff;
	const U32 b = (color >> 8) & 0xff;
	const U32 a = color & 0xff;
	UNUSED(ctx);
	/* uncomment for greyscale */
	/*const F32 y = 0.299 * r + 0.587 * g + 0.114 * b; SDL_SetRenderDrawColor(ctx->renderer, y, y, y, a);*/
	SDL_SetRenderDrawColor(ctx->renderer, r, g, b, a);
}

inline void gooey_drawLine(gooey_Ctx *ctx, I32 x1, I32 y1, I32 x2, I32 y2) {
	/* @todo account for renderRegion */
	SDL_RenderLine(ctx->renderer, x1, y1, x2, y2);
}

inline void gooey_fillRect(gooey_Ctx *ctx, Rect_I32 rect) {
	const I32 wantedL = RECT_LEFT(ctx->renderRegion) + RECT_LEFT(rect);
	const I32 maxL    = RECT_LEFT(ctx->renderRegion);
	const I32 wantedU = RECT_UP(ctx->renderRegion) + RECT_UP(rect);
	const I32 maxU    = RECT_UP(ctx->renderRegion);
	const I32 wantedR = RECT_LEFT(ctx->renderRegion) + RECT_RIGHT(rect);
	const I32 minR    = RECT_RIGHT(ctx->renderRegion);
	const I32 wantedD = RECT_UP(ctx->renderRegion) + RECT_DOWN(rect);
	const I32 minD    = RECT_DOWN(ctx->renderRegion);

	const I32 l = MAX(MAX(wantedL, maxL), 0);
	const I32 u = MAX(MAX(wantedU, maxU), 0);
	const I32 r = MAX(MIN(wantedR, minR), 0);
	const I32 d = MAX(MIN(wantedD, minD), 0);

	SDL_FRect fdst;
	fdst.x = l;
	fdst.y = u;
	fdst.w = MAX(r - l, 0);
	fdst.h = MAX(d - u, 0);

	SDL_RenderFillRect(ctx->renderer, &fdst);
}

inline void gooey_fillPolygon(
	gooey_Ctx *ctx, Vec2_I32 *vertices, size_t vertices_amount
) {
	UNUSED(ctx);
	UNUSED(vertices);
	UNUSED(vertices_amount);
	assert(0 && "unimplemented");
}
inline void gooey_drawRect(gooey_Ctx *ctx, Rect_I32 rect) {
	UNUSED(ctx);
	UNUSED(rect);
	assert(0 && "unimplemented");
}
inline void gooey_drawPolygon(
	gooey_Ctx *ctx, Vec2_I32 *vertices, size_t vertices_amount
) {
	UNUSED(ctx);
	UNUSED(vertices);
	UNUSED(vertices_amount);
	assert(0 && "unimplemented");
}

inline Bool gooey_textureIsNull(gooey_Texture *texture) {
	return texture->texture == NULL;
}

inline int gooey_makeTargetableTexture(
	gooey_Ctx *ctx,
	gooey_Texture *out_texture,
	size_t width,
	size_t height
) {
	SDL_Texture *st = SDL_CreateTexture(
		ctx->renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		width,
		height
	);
	if (st == NULL) {
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		return 1;
	};

	if (!SDL_SetTextureScaleMode(st, SDL_SCALEMODE_NEAREST)) {
		return 1;
	}

	out_texture->texture = st;
	return 0;
}

inline int gooey_makeDynamicTexture(
	gooey_Ctx *ctx,
	gooey_Texture *out_texture,
	size_t width,
	size_t height
) {
	SDL_Texture *st = SDL_CreateTexture(
		ctx->renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height
	);
	if (st == NULL) {
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		return 1;
	};

	if (!SDL_SetTextureScaleMode(st, SDL_SCALEMODE_NEAREST)) {
		return 1;
	}

	out_texture->texture = st;
	return 0;
}

inline int gooey_dynamicTextureSetPixels(
	gooey_Ctx *ctx,
	gooey_Texture dtexture,
	U32 *pixels,
	size_t width,
	size_t height
) {
	Byte *tPixels = NULL;
	Int pitch = 0;
	Int success = SDL_LockTexture(dtexture.texture, NULL, (void *)&tPixels, &pitch);
	size_t y = 0;

	UNUSED(ctx);
	if (!success) {
		printf("SDL Error %d: \"%s\"\n", success, SDL_GetError());
	}
	assert(success);

	if (pitch == (Int)(width * sizeof(U32))) {
		memcpy(tPixels, pixels, height * width * sizeof(U32));
	} else {
		for (y = 0; y < height; y++) {
			memcpy(&tPixels[y * pitch], &pixels[y * width], width * sizeof(U32));
		}
	}

	SDL_UnlockTexture(dtexture.texture);
	return 0;
}

/*
inline int gooey_dynamicTextureBeginSetPixels(
	gooey_Ctx *ctx,
	gooey_Texture dtexture,
	U32 **pixels,
	Int *pitch
) {
	Int success = SDL_LockTexture(dtexture.texture, NULL, (void *)pixels, pitch);

	UNUSED(ctx);

	if (!success) {
		printf("SDL Error %d: \"%s\"\n", success, SDL_GetError());
	}
	assert(success);

	return 0;
}

inline int gooey_dynamicTextureEndSetPixels(
	gooey_Ctx *ctx,
	gooey_Texture dtexture
) {
	SDL_UnlockTexture(dtexture.texture);
	UNUSED(ctx);
	return 0;
}*/

inline int gooey_loadTexture(
	gooey_Ctx *ctx,
	gooey_Texture *out_texture,
	const U32 *pixels,
	size_t width,
	size_t height
) {
	SDL_Texture *st = SDL_CreateTexture(
		ctx->renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STATIC,
		width,
		height
	);
	if (st == NULL) {
		return 1;
	};

	/*
	if (!SDL_SetTextureScaleMode(st, SDL_SCALEMODE_NEAREST)) {
		return 1;
	}
	*/

	if (!SDL_UpdateTexture(st, NULL, pixels, width * sizeof(U32))) {
		return 1;
	};

	out_texture->texture = st;
	return 0;
}

inline int gooey_renderTextureNoSrc(
	gooey_Ctx *ctx, gooey_Texture texture, Rect_I32 dst
) {
	float w = -1.f, h = -1.f;
	SDL_GetTextureSize(texture.texture, &w, &h);
	return gooey_renderTexture(ctx, texture, dst, make_Rect_I32(0, 0, w, h));
}

inline int gooey_renderTexture(
	gooey_Ctx *ctx, gooey_Texture texture, Rect_I32 dst, Rect_I32 src
) {
	const I32 wantedL = RECT_LEFT(ctx->renderRegion) + RECT_LEFT(dst);
	const I32 maxL = RECT_LEFT(ctx->renderRegion);
	const I32 wantedU = RECT_UP(ctx->renderRegion) + RECT_UP(dst);
	const I32 maxU = RECT_UP(ctx->renderRegion);
	const I32 wantedR = RECT_LEFT(ctx->renderRegion) + RECT_RIGHT(dst);
	const I32 minR = RECT_RIGHT(ctx->renderRegion);
	const I32 wantedD = RECT_UP(ctx->renderRegion) + RECT_DOWN(dst);
	const I32 minD = RECT_DOWN(ctx->renderRegion);

	const I32 l = MAX(MAX(wantedL, maxL), 0);
	const I32 u = MAX(MAX(wantedU, maxU), 0);
	const I32 r = MAX(MIN(wantedR, minR), 0);
	const I32 d = MAX(MIN(wantedD, minD), 0);

	const I32 wantedW = wantedR - wantedL;
	const I32 wantedH = wantedD - wantedU;

	const float xFactor = (l - wantedL) / (float)wantedW;
	const float yFactor = (u - wantedU) / (float)wantedH;

	SDL_FRect fdst = {0};
	SDL_FRect fsrc = {0};

	fdst.x = l;
	fdst.y = u;
	fdst.w = MAX(r - l, 0);
	fdst.h = MAX(d - u, 0);

	if (fdst.w < 0 || fdst.h < 0) {
		return 1;
	}

	fsrc.x = src.x + (xFactor * wantedW) / (dst.w / (float)src.w),
	fsrc.y = src.y + (yFactor * wantedH) / (dst.w / (float)src.w),
	fsrc.w = src.w * fdst.w / dst.w,
	fsrc.h = src.h * fdst.h / dst.h,

	SDL_RenderTexture(ctx->renderer, texture.texture, &fsrc, &fdst);

	return 0;
}

inline void gooey_setRenderTarget(gooey_Ctx *ctx, gooey_Texture texture) {
	SDL_SetRenderTarget(ctx->renderer, texture.texture);
}

inline void gooey_setRenderTargetToRoot(gooey_Ctx *ctx) {
	SDL_SetRenderTarget(ctx->renderer, NULL);
}

inline int gooey_destroyTexture(gooey_Ctx *ctx, gooey_Texture texture) {
	UNUSED(ctx);
	SDL_DestroyTexture(texture.texture);
	return 0;
}

#endif /* defined(GOOEY_IMPLEMENTATION) */

#endif /* !defined(GOOEY_H) */
