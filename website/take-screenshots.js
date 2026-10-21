import { chromium } from 'playwright';

(async () => {
  const browser = await chromium.launch();
  
  // Desktop screenshot
  const desktopPage = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
  await desktopPage.goto('http://localhost:4321/');
  await desktopPage.waitForLoadState('networkidle');
  await desktopPage.screenshot({ path: '/cursor/artifacts/website_home_desktop.png', fullPage: false });
  await desktopPage.close();
  
  // Mobile screenshot  
  const mobilePage = await browser.newPage({ viewport: { width: 375, height: 812 } });
  await mobilePage.goto('http://localhost:4321/');
  await mobilePage.waitForLoadState('networkidle');
  await mobilePage.screenshot({ path: '/cursor/artifacts/website_home_mobile.png', fullPage: false });
  await mobilePage.close();
  
  await browser.close();
  console.log('Screenshots saved to /cursor/artifacts/');
})();
