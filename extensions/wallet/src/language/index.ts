import VueI18n, { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
import packageJson from '../../package.json';

export const registerAwsWalletLanguage = () => {
  const { isLanguageKeyAvailable } = useLanguage();
  const locals = packageJson.kungfuConfig.language;

  if (!isLanguageKeyAvailable('awsWallet.unit')) {
    Object.keys(locals).forEach((lang) => {
      VueI18n.global.mergeLocaleMessage(lang, locals[lang]);
    });
  }
};
