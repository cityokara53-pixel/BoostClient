"use strict";

const MAX_RESPONSE_BYTES = 1024 * 1024;
const DEFAULT_TIMEOUT_MS = 6000;
const CACHE_TTL_MS = 5 * 60 * 1000;
const CACHE_MAX_ENTRIES = 1000;
const RATE_WINDOW_MS = 60 * 1000;
const RATE_LIMIT = 60;
const RATE_BUCKET_MAX = 10000;
const UUID_PATTERN = /^[0-9a-f]{32}$/i;
const SKIN_ID_PATTERN = /^[0-9a-f]{32}$/i;
const USERNAME_PATTERN = /^[A-Za-z0-9_]{3,16}$/;

const definitions = {
  mojangProfile: {
    id: "mojang-profile",
    name: "Minecraft profile lookup",
    capabilities: ["usernameLookup", "uuidLookup"],
    defaultEnabled: true,
  },
  mojangTextures: {
    id: "mojang-textures",
    name: "Minecraft skin textures",
    capabilities: ["skinPng", "preview"],
    defaultEnabled: true,
  },
  crafatar: {
    id: "crafatar",
    name: "Crafatar",
    capabilities: ["uuidLookup", "skinPng", "preview"],
    defaultEnabled: true,
  },
};

function normalizeUuid(value) {
  const normalized = String(value || "").replaceAll("-", "");
  return UUID_PATTERN.test(normalized) ? normalized.toLowerCase() : null;
}

function providerConfig(env = process.env) {
  const timeoutValue = Number(env.SKIN_API_TIMEOUT_MS);
  const timeoutMs = Number.isInteger(timeoutValue) && timeoutValue >= 500 && timeoutValue <= 15000
    ? timeoutValue
    : DEFAULT_TIMEOUT_MS;
  const priority = String(env.SKIN_API_PROVIDER_ORDER || "mojang-textures,crafatar")
    .split(",")
    .map((id) => id.trim())
    .filter((id, index, values) => id && values.indexOf(id) === index);

  return {
    timeoutMs,
    priority,
    providers: Object.values(definitions).map((provider) => {
      const key = `SKIN_API_${provider.id.toUpperCase().replaceAll("-", "_")}_ENABLED`;
      const configured = env[key];
      return {
        ...provider,
        enabled: configured === undefined ? provider.defaultEnabled : configured.toLowerCase() === "true",
      };
    }),
  };
}

function makeSkinService(options = {}) {
  const env = options.env || process.env;
  const fetchImpl = options.fetchImpl || globalThis.fetch;
  const now = options.now || Date.now;
  const cache = new Map();
  const rateBuckets = new Map();
  const config = providerConfig(env);

  function remember(key, value) {
    const time = now();
    for (const [entryKey, entry] of cache) {
      if (entry.expiresAt <= time) cache.delete(entryKey);
    }
    if (cache.size >= CACHE_MAX_ENTRIES && !cache.has(key)) {
      cache.delete(cache.keys().next().value);
    }
    cache.set(key, { value, expiresAt: time + CACHE_TTL_MS });
  }

  async function fetchLimited(url, timeoutMs = config.timeoutMs) {
    return fetchLimitedWithHeaders(url, timeoutMs, {});
  }

  async function fetchLimitedWithHeaders(url, timeoutMs, headers) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);
    try {
      const response = await fetchImpl(url, {
        signal: controller.signal,
        redirect: "error",
        headers: { Accept: "application/json, image/png", ...headers },
      });
      const length = Number(response.headers.get("content-length") || 0);
      if (length > MAX_RESPONSE_BYTES) {
        throw new Error("Provider response exceeds the size limit");
      }
      if (!response.ok) {
        const error = new Error(`Provider returned HTTP ${response.status}`);
        error.status = response.status;
        throw error;
      }
      const bytes = await readLimitedBody(response);
      if (bytes.byteLength > MAX_RESPONSE_BYTES) {
        throw new Error("Provider response exceeds the size limit");
      }
      return { response, bytes };
    } finally {
      clearTimeout(timeout);
    }
  }

  function mineSkinHeaders() {
    const apiKey = String(env.MINESKIN_API_KEY || "").trim();
    if (!apiKey) {
      const error = new Error("The MineSkin service is not configured.");
      error.status = 503;
      throw error;
    }
    return {
      Authorization: `Bearer ${apiKey}`,
      "User-Agent": "BoostClient/1.0 (Minecraft skin gallery)",
    };
  }

  async function listCatalogSkins(after) {
    const headers = mineSkinHeaders();
    if (after !== undefined && (typeof after !== "string" || !SKIN_ID_PATTERN.test(after))) {
      const error = new Error("The catalog cursor is invalid.");
      error.status = 400;
      throw error;
    }
    const url = new URL("https://api.mineskin.org/v2/skins");
    if (after) url.searchParams.set("after", after.toLowerCase());
    const cacheKey = `catalog:${after || "first"}`;
    const cached = cache.get(cacheKey);
    if (cached && cached.expiresAt > now()) return cached.value;

    const { bytes } = await fetchLimitedWithHeaders(url, config.timeoutMs, headers);
    const payload = readJson(bytes);
    if (!Array.isArray(payload.skins)) {
      throw new Error("MineSkin returned an invalid catalog page.");
    }
    const skins = payload.skins.flatMap((skin) => {
      if (!skin || !SKIN_ID_PATTERN.test(String(skin.uuid || ""))) return [];
      const visibility = ["public", "unlisted", "private"].includes(skin.visibility) ? skin.visibility : "public";
      const variant = ["classic", "slim", "unknown"].includes(skin.variant) ? skin.variant : "unknown";
      return [{
        uuid: skin.uuid.toLowerCase(),
        name: typeof skin.name === "string" ? skin.name.slice(0, 80) : "",
        variant,
        visibility,
        views: Number.isFinite(skin.views) ? Math.max(0, Math.floor(skin.views)) : 0,
      }];
    }).filter((skin) => skin.visibility === "public");

    let nextAfter = null;
    if (typeof payload.links?.next === "string") {
      try {
        const nextUrl = new URL(payload.links.next, "https://api.mineskin.org");
        if (nextUrl.origin === "https://api.mineskin.org" && nextUrl.pathname === "/v2/skins") {
          const candidate = nextUrl.searchParams.get("after");
          if (candidate && SKIN_ID_PATTERN.test(candidate)) nextAfter = candidate.toLowerCase();
        }
      } catch {
        throw new Error("MineSkin returned an invalid catalog page link.");
      }
    }
    const result = { skins, nextAfter };
    remember(cacheKey, result);
    return result;
  }

  async function downloadCatalogSkin(skinId) {
    const headers = mineSkinHeaders();
    if (typeof skinId !== "string" || !SKIN_ID_PATTERN.test(skinId)) {
      const error = new Error("A valid MineSkin skin UUID is required.");
      error.status = 400;
      throw error;
    }
    const normalizedId = skinId.toLowerCase();
    const cacheKey = `catalog-skin:${normalizedId}`;
    const cached = cache.get(cacheKey);
    if (cached && cached.expiresAt > now()) return cached.value;

    const detailUrl = `https://api.mineskin.org/v2/skins/${normalizedId}`;
    const { bytes: detailBytes } = await fetchLimitedWithHeaders(detailUrl, config.timeoutMs, headers);
    const detail = readJson(detailBytes).skin;
    if (!detail || String(detail.uuid || "").toLowerCase() !== normalizedId || detail.visibility !== "public") {
      const error = new Error("The requested public MineSkin skin was not found.");
      error.status = 404;
      throw error;
    }
    const skinUrl = new URL(String(detail.texture?.url?.skin || ""));
    if (skinUrl.protocol !== "https:" || skinUrl.hostname !== "textures.minecraft.net" ||
        !/^\/texture\/[0-9a-f]{32,}$/i.test(skinUrl.pathname) || skinUrl.username || skinUrl.password) {
      throw new Error("MineSkin returned an unsupported texture URL.");
    }
    const { response, bytes } = await fetchLimited(skinUrl);
    const contentType = (response.headers.get("content-type") || "").split(";")[0].trim().toLowerCase();
    if (contentType !== "image/png" || bytes.length < 8 ||
        !bytes.subarray(0, 8).equals(Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]))) {
      throw new Error("MineSkin returned an invalid skin PNG.");
    }
    const result = { bytes, variant: ["classic", "slim"].includes(detail.variant) ? detail.variant : "classic" };
    remember(cacheKey, result);
    return result;
  }

  async function readLimitedBody(response) {
    if (!response.body?.getReader) {
      return Buffer.from(await response.arrayBuffer());
    }
    const reader = response.body.getReader();
    const chunks = [];
    let size = 0;
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      size += value.byteLength;
      if (size > MAX_RESPONSE_BYTES) {
        await reader.cancel();
        throw new Error("Provider response exceeds the size limit");
      }
      chunks.push(Buffer.from(value));
    }
    return Buffer.concat(chunks, size);
  }

  function readJson(bytes) {
    try {
      return JSON.parse(new TextDecoder().decode(bytes));
    } catch {
      throw new Error("Provider returned invalid JSON");
    }
  }

  function checkRateLimit(key) {
    const time = now();
    const bucket = rateBuckets.get(key);
    if (!bucket || bucket.resetAt <= time) {
      if (rateBuckets.size >= RATE_BUCKET_MAX) {
        for (const [entryKey, entry] of rateBuckets) {
          if (entry.resetAt <= time) rateBuckets.delete(entryKey);
        }
        if (rateBuckets.size >= RATE_BUCKET_MAX) {
          return { allowed: false, retryAfter: Math.ceil(RATE_WINDOW_MS / 1000) };
        }
      }
      rateBuckets.set(key, { count: 1, resetAt: time + RATE_WINDOW_MS });
      return { allowed: true, retryAfter: 0 };
    }
    bucket.count += 1;
    return {
      allowed: bucket.count <= RATE_LIMIT,
      retryAfter: Math.max(1, Math.ceil((bucket.resetAt - time) / 1000)),
    };
  }

  async function getProfileByUuid(uuid) {
    const normalizedUuid = normalizeUuid(uuid);
    if (!normalizedUuid) {
      const error = new Error("A valid Minecraft UUID is required.");
      error.status = 400;
      throw error;
    }
    if (!config.providers.some((provider) => provider.id === "mojang-profile" && provider.enabled)) {
      const error = new Error("Minecraft profile lookup is disabled.");
      error.status = 503;
      throw error;
    }
    const cacheKey = `profile:${normalizedUuid}`;
    const cached = cache.get(cacheKey);
    if (cached && cached.expiresAt > now()) {
      return cached.value;
    }

    const sessionUrl = `https://sessionserver.mojang.com/session/minecraft/profile/${normalizedUuid}?unsigned=false`;
    const { bytes } = await fetchLimited(sessionUrl);
    const profile = readJson(bytes);
    const profileUuid = normalizeUuid(profile.id);
    const texturesProperty = Array.isArray(profile.properties)
      ? profile.properties.find((property) => property.name === "textures")
      : undefined;
    if (!profileUuid || !profile.name || !texturesProperty?.value) {
      const error = new Error("Minecraft profile has no usable skin data.");
      error.status = 404;
      throw error;
    }

    let textureData;
    try {
      textureData = JSON.parse(Buffer.from(texturesProperty.value, "base64").toString("utf8"));
    } catch {
      throw new Error("Minecraft profile returned invalid skin metadata.");
    }
    const skin = textureData.textures?.SKIN;
    if (!skin?.url) {
      const error = new Error("Minecraft profile does not have a skin.");
      error.status = 404;
      throw error;
    }
    const textureUrl = new URL(skin.url);
    if (textureUrl.protocol !== "https:" || textureUrl.hostname !== "textures.minecraft.net" ||
        !/^\/texture\/[0-9a-f]{32,}$/i.test(textureUrl.pathname)) {
      throw new Error("Minecraft profile returned an unsupported skin URL.");
    }

    const value = {
      uuid: profileUuid,
      username: profile.name,
      variant: skin.metadata?.model === "slim" ? "slim" : "classic",
      provider: "mojang-profile",
      downloadUrl: `/api/skins/skin?uuid=${profileUuid}`,
    };
    remember(cacheKey, value);
    return value;
  }

  async function lookupUsername(username) {
    if (typeof username !== "string" || !USERNAME_PATTERN.test(username)) {
      const error = new Error("Enter a valid Minecraft username (3–16 letters, numbers, or underscores).");
      error.status = 400;
      throw error;
    }
    const cacheKey = `username:${username.toLowerCase()}`;
    const cached = cache.get(cacheKey);
    if (cached && cached.expiresAt > now()) {
      return cached.value;
    }

    const providers = config.providers.filter((provider) => provider.id === "mojang-profile" && provider.enabled);
    if (providers.length === 0) {
      const error = new Error("Username lookup provider is not configured.");
      error.status = 503;
      throw error;
    }
    const encodedUsername = encodeURIComponent(username);
    const lookups = [
      `https://api.minecraftservices.com/minecraft/profile/lookup/name/${encodedUsername}`,
      `https://api.mojang.com/users/profiles/minecraft/${encodedUsername}`,
    ];
    const failures = [];
    for (const lookupUrl of lookups) {
      try {
        const { bytes } = await fetchLimited(lookupUrl);
        const profile = readJson(bytes);
        const uuid = normalizeUuid(profile.id);
        if (!uuid || !profile.name) {
          throw new Error("Profile lookup returned invalid profile data.");
        }
        const value = await getProfileByUuid(uuid);
        remember(cacheKey, value);
        return value;
      } catch (error) {
        if (error.status === 400 || error.status === 404) {
          throw error;
        }
        failures.push(error);
      }
    }
    const rateLimited = failures.find((error) => error.status === 429);
    if (rateLimited) {
      rateLimited.status = 503;
      rateLimited.retryAfter = 30;
      throw rateLimited;
    }
    const error = new Error("Minecraft profile providers are temporarily unavailable.");
    error.status = 503;
    error.providers = failures.map((failure) => failure.message);
    throw error;
  }

  async function downloadSkin(uuid) {
    const normalizedUuid = normalizeUuid(uuid);
    if (!normalizedUuid) {
      const error = new Error("A valid Minecraft UUID is required.");
      error.status = 400;
      throw error;
    }
    const order = config.priority.filter((id) => id === "mojang-textures" || id === "crafatar");
    for (const id of ["mojang-textures", "crafatar"]) {
      if (!order.includes(id)) order.push(id);
    }
    const enabled = new Set(config.providers.filter((provider) => provider.enabled).map((provider) => provider.id));
    const failures = [];
    if (!order.some((providerId) => enabled.has(providerId))) {
      const error = new Error("No skin image provider is enabled.");
      error.status = 503;
      throw error;
    }
    for (const providerId of order) {
      if (!enabled.has(providerId)) continue;
      try {
        const url = providerId === "crafatar"
          ? `https://crafatar.com/skins/${normalizedUuid}`
          : await getTextureUrl(normalizedUuid);
        const { response, bytes } = await fetchLimited(url);
        const contentType = (response.headers.get("content-type") || "").split(";")[0].trim().toLowerCase();
        if (contentType !== "image/png") {
          throw new Error(`${providerId} returned a non-PNG skin.`);
        }
        if (bytes.length > MAX_RESPONSE_BYTES || bytes.length < 8 ||
            !bytes.subarray(0, 8).equals(Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]))) {
          throw new Error(`${providerId} returned an invalid PNG.`);
        }
        return { bytes, provider: providerId };
      } catch (error) {
        failures.push(error);
      }
    }
    const rateLimited = failures.find((error) => error.status === 429);
    const error = new Error(rateLimited ? "Skin providers are rate-limiting requests." : "All skin providers failed.");
    error.status = rateLimited ? 503 : 502;
    if (rateLimited) error.retryAfter = 30;
    throw error;
  }

  async function getTextureUrl(uuid) {
    const normalizedUuid = normalizeUuid(uuid);
    if (!normalizedUuid) {
      const error = new Error("A valid Minecraft UUID is required.");
      error.status = 400;
      throw error;
    }
    const { bytes } = await fetchLimited(
      `https://sessionserver.mojang.com/session/minecraft/profile/${normalizedUuid}?unsigned=false`,
    );
    const profile = readJson(bytes);
    const value = profile.properties?.find((property) => property.name === "textures")?.value;
    if (!value) throw new Error("Minecraft profile has no texture property.");
    let decoded;
    try {
      decoded = JSON.parse(Buffer.from(value, "base64").toString("utf8"));
    } catch {
      throw new Error("Minecraft profile returned invalid texture metadata.");
    }
    const url = decoded.textures?.SKIN?.url;
    if (!url) throw new Error("Minecraft profile has no skin texture.");
    const parsed = new URL(url);
    if (parsed.protocol !== "https:" || parsed.hostname !== "textures.minecraft.net" ||
        !/^\/texture\/[0-9a-f]{32,}$/i.test(parsed.pathname)) {
      throw new Error("Minecraft profile returned an unsupported skin URL.");
    }
    return parsed.href;
  }

  return {
    checkRateLimit,
    config,
    downloadSkin,
    downloadCatalogSkin,
    getProfileByUuid,
    listCatalogSkins,
    lookupUsername,
    listProviders() {
      return config.providers.map(({ id, name, capabilities, enabled }) => ({ id, name, capabilities, enabled }));
    },
  };
}

function sendJson(response, status, body, cacheControl = "no-store") {
  response.setHeader("Content-Type", "application/json; charset=utf-8");
  response.setHeader("Cache-Control", cacheControl);
  response.setHeader("X-Content-Type-Options", "nosniff");
  response.status(status).json(body);
}

function requestIp(request) {
  const forwarded = request.headers["x-forwarded-for"];
  const ip = typeof forwarded === "string" ? forwarded.split(",")[0].trim() : "";
  return ip.slice(0, 64) || "unknown";
}

function guardRequest(request, response, service) {
  response.setHeader("Access-Control-Allow-Origin", process.env.SKIN_API_CORS_ORIGIN || "*");
  response.setHeader("Vary", "Origin");
  response.setHeader("X-Content-Type-Options", "nosniff");
  if (request.method !== "GET") {
    sendJson(response, 405, { error: "Method not allowed." });
    return false;
  }
  const limit = service.checkRateLimit(requestIp(request));
  if (!limit.allowed) {
    response.setHeader("Retry-After", String(limit.retryAfter));
    sendJson(response, 429, { error: "Rate limit exceeded. Try again shortly." });
    return false;
  }
  return true;
}

function sendError(response, error) {
  const status = Number.isInteger(error.status) && error.status >= 400 && error.status <= 599 ? error.status : 502;
  if (Number.isInteger(error.retryAfter) && error.retryAfter > 0) {
    response.setHeader("Retry-After", String(error.retryAfter));
  }
  sendJson(response, status, { error: error.message || "Skin provider request failed." });
}

module.exports = {
  MAX_RESPONSE_BYTES,
  makeSkinService,
  normalizeUuid,
  providerConfig,
  guardRequest,
  sendError,
  sendJson,
};
