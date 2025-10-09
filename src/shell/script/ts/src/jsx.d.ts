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
        onClick?: (key: number) => void;
        onMouseEnter?: () => void;
        onMouseLeave?: () => void;
        onMouseDown?: () => void;
        onMouseUp?: () => void;
        onMouseMove?: (x: number, y: number) => void;
        justifyContent?: 'start' | 'center' | 'end' | 'space-between' | 'space-around' | 'space-evenly';
        alignItems?: 'start' | 'center' | 'end' | 'stretch';
        horizontal?: boolean;
        children?: React.ReactNode | React.ReactNode[];
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        autoSize?: boolean;
        gap?: number;
        flexGrow?: number;
      },
      text: {
        text?: string[] | string;
        fontSize?: number;
        color?: string;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        maxWidth?: number;
      },
      img: {
        svg?: string;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
      },
      spacer: {
        size?: number;
        key?: string | number;
      }
    }
  }
}
