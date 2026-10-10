import { breeze_paint } from "mshell";

type AnimationCurve = {
  duration: number;
  easing: 'mutation' | 'linear' | 'ease_in' | 'ease_out' | 'ease_in_out';
  vars: string[];
};

/** 任何 widget 都能参与 flex 布局, 这些属性对 flex/text/textbox/img 都有效。 */
type FlexItemProps = {
  flexGrow?: number;
  flexShrink?: number;
  minWidth?: number;
  minHeight?: number;
  alignSelf?: 'auto' | 'start' | 'center' | 'end' | 'stretch' | 'baseline';
};

declare module 'react' {
  namespace JSX {
    interface IntrinsicElements {
      flex: FlexItemProps & {
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
        cursor?: 'arrow' | 'text' | 'hand' | 'crosshair' | 'size_h' | 'size_v' | 'auto';
        autoSize?: boolean;
        gap?: number;
        maxHeight?: number;
        enableScrolling?: boolean;
        enableChildClipping?: boolean;
        cropOverflow?: boolean;
        opacity?: number;
        animationCurve?: AnimationCurve;
        flexWrap?: 'nowrap' | 'wrap' | 'wrap-reverse';
        reverse?: boolean;
        scrollBarColor?: string;
        scrollBarWidth?: number;
        scrollBarRadius?: number;
        scrollBarMargin?: number;
      },
      text: FlexItemProps & {
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
        cursor?: 'arrow' | 'text' | 'hand' | 'crosshair' | 'size_h' | 'size_v' | 'auto';
        flexGrow?: number;
        flexShrink?: number;
        // 换行宽度上限: 配合 textOverflow='ellipsis' 就是单行省略。
        maxWidth?: number;
        fontFamily?: 'main' | 'monospace' | 'fallback';
        animationCurve?: AnimationCurve;
        lineHeight?: number;
        textOverflow?: 'wrap' | 'clip' | 'ellipsis';
      },
      textbox: FlexItemProps & {
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
        cursor?: 'arrow' | 'text' | 'hand' | 'crosshair' | 'size_h' | 'size_v' | 'auto';
        flexGrow?: number;
        flexShrink?: number;
      },
      img: FlexItemProps & {
        svg?: string;
        key?: string | number;
        animatedVars?: string[];
        x?: number;
        y?: number;
        width?: number;
        height?: number;
        cursor?: 'arrow' | 'text' | 'hand' | 'crosshair' | 'size_h' | 'size_v' | 'auto';
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
