import { breeze_paint } from "mshell";

declare module 'react' {
  namespace JSX {
    interface IntrinsicElements {
      flex: {
        padding?: number;
        paddingTop?: number;
        paddingRight?: number;
        paddingBottom?: number;
        paddingLeft?: number;
        backgroundColor?: string;
        borderColor?: string;
        borderRadius?: number;
        borderWidth?: number;
        backgroundPaint?: breeze_paint;
        borderPaint?: breeze_paint;
        onClick?: () => void;
        onMouseEnter?: () => void;
        horizontal?: boolean;
        children?: React.ReactNode | React.ReactNode[];
        key?: string | number;
      },
      text: {
        text?: string[] | string;
        fontSize?: number;
        color?: string;
        key?: string | number;
      }
    }
  }
}
