# Fomawrite Website

Launch website for Fomawrite, a calm Markdown editor for macOS.

## Build and Deploy

### Local Development

```bash
npm install
npm run dev
```

Open [http://localhost:4321](http://localhost:4321) in your browser.

### Build for Production

```bash
npm ci
npm run build
```

The static site is built to `dist/`.

### Deploy to Cloudflare Pages

Install Wrangler CLI:

```bash
npm install -g wrangler
```

Deploy:

```bash
wrangler pages deploy dist --project-name fomawrite
```

Or connect your GitHub repository to Cloudflare Pages for automatic deployments:

1. Go to [dash.cloudflare.com](https://dash.cloudflare.com) → Pages
2. Create a new project
3. Connect your GitHub repository
4. Set build command: `npm ci && npm run build`
5. Set build output directory: `dist`
6. Deploy

## Configuration

### Waitlist Endpoint

The waitlist form currently shows a success message without sending data to a backend.

To connect a real backend:

1. Set up your waitlist endpoint (e.g., Mailchimp, ConvertKit, custom API)
2. Edit `src/pages/waitlist.astro`
3. Uncomment the fetch code in the form submit handler
4. Update the `endpoint` variable with your API URL
5. Add any required authentication headers

Example:

```javascript
const endpoint = 'https://api.example.com/waitlist';
const response = await fetch(endpoint, {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Authorization': 'Bearer YOUR_API_KEY' // Don't commit this!
  },
  body: JSON.stringify(data),
});
```

**Important:** Never commit API keys to the repository. Use Cloudflare Pages environment variables:

1. Go to your Cloudflare Pages project → Settings → Environment variables
2. Add your API keys as encrypted variables
3. Access them in your edge functions or build process

### Analytics

To add analytics (Plausible, Fathom, Google Analytics):

1. Edit `src/layouts/Layout.astro`
2. Add the analytics script tag before the closing `</head>` tag

Example for Plausible:

```astro
<script defer data-domain="fomawrite.app" src="https://plausible.io/js/script.js"></script>
```

### Custom Domain

To use a custom domain with Cloudflare Pages:

1. Go to your Cloudflare Pages project → Custom domains
2. Add your domain (e.g., `fomawrite.app`)
3. Follow the DNS configuration instructions
4. Update `astro.config.mjs` with your domain:

```javascript
export default defineConfig({
  site: 'https://fomawrite.app',
  // ...
});
```

## Pages

- **Home** (`/`) - Hero, features, status, CTA
- **Changelog** (`/changelog`) - Development history from recent cycles
- **Docs** (`/docs`) - Getting started and usage guide
- **About** (`/about`) - Mission and project information
- **Waitlist** (`/waitlist`) - Email collection form

## Technology

- **Astro** - Static site generator
- **TypeScript** - Type safety
- Dark theme inspired by [terax.app](https://terax.app) and [getbb.app](https://getbb.app)
- Responsive design
- SEO optimized

## Project Structure

```
website/
├── public/           # Static assets
│   ├── favicon.svg
│   ├── og-image.png
│   └── robots.txt
├── src/
│   ├── components/   # Reusable components
│   │   ├── Nav.astro
│   │   └── Footer.astro
│   ├── layouts/      # Page layouts
│   │   └── Layout.astro
│   └── pages/        # Routes (file-based routing)
│       ├── index.astro
│       ├── changelog.astro
│       ├── docs.astro
│       ├── about.astro
│       └── waitlist.astro
├── astro.config.mjs  # Astro configuration
├── package.json
└── README.md
```

## License

MIT (same as Fomawrite)
