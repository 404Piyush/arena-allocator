# Deployment

This repo ships with a static landing page under `site/` and
a `netlify.toml` that auto-deploys it. The site is already
deployed to Netlify and has its custom domain set. This
document is the runbook if you ever need to redeploy from
scratch.

## Netlify state

- Netlify URL: https://arena-allocator.netlify.app
- Custom domain: arena-allocator.404piyush.me

The Netlify site is linked to this GitHub repo. Every push
to `main` redeploys automatically.

## Redeploy from scratch

1. Go to https://app.netlify.com/start and click
   **Import an existing project**.
2. Pick **GitHub** and `404Piyush/arena-allocator`.
3. Netlify will detect `netlify.toml`. Confirm:
   - Build command: empty
   - Publish directory: `site`
4. Click **Deploy site**.

## Add the custom domain

In Netlify, go to **Site settings**, **Domain management**,
**Add custom domain**, and enter `arena-allocator.404piyush.me`.

## Local preview

```sh
cd site
python3 -m http.server 8000
# open http://localhost:8000
```
