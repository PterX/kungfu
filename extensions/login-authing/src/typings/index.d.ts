import {
  LoginTokenResponseDataDto,
  UserDto,
} from 'authing-node-sdk/dist/models';

export type Stage = 'dev' | 'alpha' | 'prod';

export interface AuthingAppConfig {
  appId: string;
  appHost: string;
  appSecret: string;
}

export interface Credential extends LoginTokenResponseDataDto {
  expires_in: number;
  gen_time: number;
}

export interface UserInfo extends UserDto {
  sub?: string;
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  [key: string]: any;
}

export type Credentials = Partial<Record<Stage, Credential>>;
