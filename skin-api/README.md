# BoostClient Skin API

This is a Vercel-compatible Node.js API for resolving Minecraft profile skins, serving MineSkin's paginated public catalog, and returning validated PNG images. It uses fixed upstream hosts and paths only; it is not an arbitrary URL proxy.

## Providers and capabilities

| Provider | Capabilities | Credentials |
| --- | --- | --- |
| [MineSkin](https://docs.mineskin.org/docs/mineskin-api/mineskin-api/) | Paginated public skin catalog; catalog-skin metadata and PNG | `MINESKIN_API_KEY`, stored only in Vercel |
| Minecraft profile services (Mojang/Minecraft Services) | Username lookup, UUID profile lookup, skin metadata | None |
| Mojang texture host | Skin PNG retrieval and preview, first image provider by default | None |
| Crafatar | UUID skin PNG retrieval and preview; fallback image provider | None |

BoostClient's **Skin Studio** is a custom Qt interface with Free Skins, My Skins, Favorites, and Recently Used views; a responsive card grid; local search and category filters; and the existing interactive 3D preview. Free Skins starts with the 1,000 PNG files in `launcher/resources/skins/BoostClient_Skins.zip`. The archive is embedded in the launcher, so the gallery does not depend on the developer's original ZIP path or an internet connection. On first use, supported 64×64 and legacy 64×32 PNGs are validated, copied into the user's persistent skin cache, and indexed; later launches reuse that index. Search covers filenames and available folder metadata, while category filtering uses actual archive folders. The supplied pack contains only the `Mobs` category and no popularity ratings, so unsupported theme filters and Popular correctly return empty states rather than fabricated results. Users can also choose a PNG or another ZIP pack through **Import Skin**. Favorites and recent selections persist locally. Selecting a card updates the real 3D preview without applying it until the user chooses an action. Applying to Minecraft services still requires an authenticated account; offline accounts can only save locally. The MineSkin key is sent only from Vercel to MineSkin; it is never returned to the client or embedded in BoostClient. Username/UUID **Online Lookup** still uses the configured Vercel API. The launcher no longer embeds the MCCatalog website.

To add a configured skin pack at runtime, set `SkinArchivePath` in the launcher configuration or set the `BOOSTCLIENT_SKIN_ARCHIVE` environment variable to a ZIP path. Configured packs are added to the persistent index; they do not replace the bundled pack. Importing a pack in the Skin Studio also adds it to the same index.

## Routes

- `GET /api/providers` — provider capability and configuration status.
- `GET /api/skins/catalog` — a page of public MineSkin catalog entries; accepts the opaque `after` cursor from the previous response.
- `GET /api/skins/catalog-skin?uuid=<uuid>` — the selected catalog skin PNG. The API resolves and validates MineSkin's texture URL server-side.
- `GET /api/skins/search?username=<name>` — Minecraft username lookup.
- `GET /api/skins/search?uuid=<uuid>` — Minecraft UUID profile lookup.
- `GET /api/skins/skin?uuid=<uuid>` — validated Minecraft profile skin PNG.

The API catalog routes remain available for server-side integrations, but the launcher's initial Free Skins gallery is populated from the bundled ZIP. The launcher stores selected images locally. Offline accounts can save skins in their library, but cannot upload or sync them to Minecraft services. Applying a skin to an authenticated Microsoft account continues through the launcher's existing Minecraft Services flow.

## Vercel environment variables

Required configuration:

| Variable | Default | Purpose |
| --- | --- | --- |
| `MINESKIN_API_KEY` | — | MineSkin bearer credential. Add it in Vercel Project Settings → Environment Variables; never put it in source, launcher settings, or a URL. |

Optional configuration:

| Variable | Default | Purpose |
| --- | --- | --- |
| `SKIN_API_PROVIDER_ORDER` | `mojang-textures,crafatar` | Image-provider preference and fallback order. |
| `SKIN_API_MOJANG_PROFILE_ENABLED` | `true` | Enable username/UUID profile resolution. |
| `SKIN_API_MOJANG_TEXTURES_ENABLED` | `true` | Enable Mojang texture image retrieval. |
| `SKIN_API_CRAFATAR_ENABLED` | `true` | Enable Crafatar image fallback. |
| `SKIN_API_TIMEOUT_MS` | `6000` | Per-upstream timeout, bounded to 500–15000 ms. |
| `SKIN_API_CORS_ORIGIN` | `*` | Optional CORS origin for browser clients. |

Provider enable variables accept `true` or `false`. If all image providers are disabled, profile skin image requests return an unavailable response.

## Security and operational limits

- Request inputs are validated; upstream URLs are fixed and redirects are rejected.
- Upstream requests have timeouts; response bodies are limited to 1 MiB and PNG signatures/content types are checked.
- The API applies a 60-request-per-minute per-IP in-memory limiter and a five-minute per-instance lookup cache. Vercel serverless instances do not share memory, so this limiter is best-effort, not a globally enforced quota. Use a durable shared rate limiter before relying on it for abuse prevention at scale.
- Responses do not include environment variables, credentials, or upstream error details.
- MineSkin authorization is added only by the Vercel server when calling MineSkin. It is not forwarded to the texture host or sent to BoostClient.
- API requests and response payloads must not be logged with authorization data. Catalog routes require no client-side credentials.

## Manual Vercel deployment

There is no Vercel project configuration or live deployment in this repository yet. Deployment requires the developer's own Vercel account and authentication:

1. Create or import the repository as a Vercel project in the Vercel dashboard.
2. Set the project **Root Directory** to `skin-api` and select Node.js 20 or newer.
3. In **Project Settings → Environment Variables**, add `MINESKIN_API_KEY` and paste the key from `https://account.mineskin.org/keys`. Select the required environments and save. Do not send the key in chat or add it to repository files. Keep the variable on Vercel only.
4. Deploy from the dashboard, or use the Vercel CLI after manually authenticating:

   ```powershell
   Set-Location 'C:\Users\cityo\Downloads\BoostClient\skin-api'
   vercel
   ```

5. Verify the deployment without exposing configuration:

   ```powershell
   Invoke-RestMethod 'https://YOUR-PROJECT.vercel.app/api/providers'
   Invoke-RestMethod 'https://YOUR-PROJECT.vercel.app/api/skins/search?username=YOUR_TEST_USERNAME'
   ```

   The first response should report `catalog: true`. The catalog route should return public entries when `MINESKIN_API_KEY` is configured. Do not include keys in URLs or commands.

6. In BoostClient, configure the public Vercel project origin (for example `https://your-project.vercel.app`) for **Online Lookup**. The local bundled skin gallery works independently of Vercel; the Vercel origin is not a secret.

## Local development secrets template

Do not create a `.env` file containing the MineSkin key in the launcher workspace. The app needs only the public Vercel origin; Vercel holds and uses the secret.

## Tests

Run from this folder with Node.js 20 or newer:

```powershell
node --test
```
