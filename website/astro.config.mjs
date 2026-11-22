import { defineConfig } from 'astro/config';
import sitemap from '@astrojs/sitemap';

export default defineConfig({
  site: 'https://fomawrite.app',
  integrations: [sitemap()],
  build: {
    format: 'file'
  }
});
