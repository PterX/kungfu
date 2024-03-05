import {
  WalletCurrencyEnum,
  WalletStatusEnum,
  WalletTransTypeEnum,
  WalletTransStatusEnum,
  WalletInvoiceTypeEnum,
  WalletInvoiceStatusEnum,
} from '../typings/enum';

export interface WalletData {
  lago_id: string;
  lago_customer_id: string;
  external_customer_id: string;
  status: WalletStatusEnum;
  currency: WalletCurrencyEnum;
  name: string;
  rate_amount: number | null;
  balance: number | null;
  credits_balance: string;
  balance_cents: number;
  consumed_credits: string;
  created_at: number;
  expiration_at: number | null;
  last_balance_sync_at: number | null;
  last_consumed_credit_at: number | null;
  terminated_at: number | null;
  expiration_date: number | null;
}

export interface WalletTransData {
  lago_id: string;
  lago_wallet_id: string;
  status: WalletTransStatusEnum;
  transaction_type: WalletTransTypeEnum;
  amount: number | null;
  credit_amount: number | null;
  settled_at: number | null;
  created_at: number | null;
}

export interface WalletCustomerData {
  lago_id: string;
  external_id: string;
  name: null;
  sequential_id: number;
  slug: string;
  created_at: string;
  updated_at: string;
  country: 'CN';
  address_line1: null;
  address_line2: null;
  state: null;
  zipcode: null;
  email: null;
  city: null;
  url: null;
  phone: string;
  logo_url: null;
  legal_name: null;
  legal_number: null;
  currency: WalletCurrencyEnum;
  tax_identification_number: null;
  timezone: null;
  applicable_timezone: 'UTC';
  net_payment_term: null;
  external_salesforce_id: null;
  billing_configuration: {
    invoice_grace_period: null;
    payment_provider: null;
    vat_rate: null;
    document_locale: null;
  };
}

export interface WalletInvoiceData {
  lago_id: string;
  sequential_id: number;
  number: string;
  issuing_date: string;
  payment_due_date: string;
  net_payment_term: number;
  invoice_type: WalletInvoiceTypeEnum;
  status: WalletInvoiceStatusEnum;
  payment_status: WalletInvoiceStatusEnum;
  currency: WalletCurrencyEnum;
  fees_amount_cents: number;
  taxes_amount_cents: number;
  coupons_amount_cents: number;
  credit_notes_amount_cents: number;
  sub_total_excluding_taxes_amount_cents: number;
  sub_total_including_taxes_amount_cents: number;
  total_amount_cents: number;
  prepaid_credit_amount_cents: number;
  file_url: string | null;
  version_number: number;
  legacy: boolean;
  amount_currency: WalletCurrencyEnum;
  vat_amount_currency: WalletCurrencyEnum;
  credit_amount_currency: WalletCurrencyEnum;
  total_amount_currency: WalletCurrencyEnum;
  amount: number;
  amount_cents: number;
  credit_amount_cents: number;
  vat_amount_cents: number;
  sub_total_vat_excluded_amount_cents: number;
  sub_total_vat_included_amount_cents: number;
  customer: WalletCustomerData;
}

export interface WalletTransDataForShow extends WalletTransData {
  isPending: boolean;
}
