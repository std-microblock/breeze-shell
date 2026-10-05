import { breeze_paint } from "mshell";

type AnimationCurve = {
  duration: number;
  easing: 'mutation' | 'linear' | 'ease_in' | 'ease_out' | 'ease_in_out';
  vars: string[];
};

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
        justifyContent?: 'start' | 'center' | 'end' | 'space-between' | 'space-around' | 'space-evenly' | 'free';
        alignItems?: 'start' | 'center' | 'end' | 'stretch' | 'free';
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
        flexShrink?: number;
        maxHeight?: number;
        enableScrolling?: boolean;
        enableChildClipping?: boolean;
        cropOverflow?: boolean;
        opacity?: number;
        animationCurve?: AnimationCurve;
      },
      text: {
        text?: string[] | string;
        fontSize?: number;
        fontWeight?: number;
        color?: string;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        flexGrow?: number;
        flexShrink?: number;
        maxWidth?: number;
        fontFamily?: 'main' | 'monospace' | 'fallback';
        animationCurve?: AnimationCurve;
      },
      textbox: {
        text?: string;
        value?: string;
        placeholder?: string;
        fontSize?: number;
        fontWeight?: number;
        paddingX?: number;
        paddingY?: number;
        borderRadius?: number;
        minHeight?: number;
        preferredMultilineHeight?: number;
        lineHeightMultiplier?: number;
        multiline?: boolean;
        readonly?: boolean;
        disabled?: boolean;
        backgroundColor?: string;
        readonlyBackgroundColor?: string;
        disabledBackgroundColor?: string;
        borderColor?: string;
        focusBorderColor?: string;
        textColor?: string;
        disabledTextColor?: string;
        placeholderColor?: string;
        selectionColor?: string;
        caretColor?: string;
        compositionUnderlineColor?: string;
        onChange?: (value: string) => void;
        onFocus?: () => void;
        onBlur?: () => void;
        onKeyDown?: (key: number, shiftKey: boolean, ctrlKey: boolean, altKey: boolean, metaKey: boolean) => boolean;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        flexGrow?: number;
        flexShrink?: number;
      },
      img: {
        svg?: string;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        flexGrow?: number;
        flexShrink?: number;
      },
      spacer: {
        size?: number;
        key?: string | number;
      }
    }
  }
}
