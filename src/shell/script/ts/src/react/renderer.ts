import Reconciler, { HostConfig } from 'react-reconciler'
import * as shell from "mshell"
import React from 'react';

const getSetFactory = (fieldname: string) => {
    return {
        set: (instance: shell.breeze_ui.js_widget, value: any) => {
            const v = Array.isArray(value) ? value : [value];
            instance.downcast()['set_' + fieldname](...v);
        },
        get: (instance: shell.breeze_ui.js_widget) => {
            return instance.downcast()['get_' + fieldname]();
        }
    }
}

const getSetFactoryAutoRepeat = (fieldname: string, repeatTime: number = 4) => {
    return {
        set: (instance: shell.breeze_ui.js_widget, value: any) => {
            const v = Array.isArray(value) ? value : [value];
            while (v.length < repeatTime) {
                v.push(v[v.length - 1]);
            }
            instance.downcast()['set_' + fieldname](...v);
        },
        get: (instance: shell.breeze_ui.js_widget) => {
            return instance.downcast()['get_' + fieldname]();
        }
    }
}

const getSetFactoryColor = (fieldname: string) => {
    return {
        set: (instance: shell.breeze_ui.js_text_widget, value: string) => {
            instance['set_' + fieldname](hex_to_rgba(value));
        },
        get: (instance: shell.breeze_ui.js_text_widget) => {
            return rgba_to_hex(instance['get_' + fieldname]());
        }
    }
}

const hex_to_rgba = (color: string): [
    number, number, number, number
] => {
    if (color.startsWith('#')) {
        const hex = color.slice(1);
        if (hex.length === 6) {
            return [
                parseInt(hex.slice(0, 2), 16) / 255,
                parseInt(hex.slice(2, 4), 16) / 255,
                parseInt(hex.slice(4, 6), 16) / 255,
                1
            ];
        } else if (hex.length === 8) {
            return [
                parseInt(hex.slice(0, 2), 16) / 255,
                parseInt(hex.slice(2, 4), 16) / 255,
                parseInt(hex.slice(4, 6), 16) / 255,
                parseInt(hex.slice(6, 8), 16) / 255
            ];
        }
    }
}


const rgba_to_hex = (rgba: [number, number, number, number]) => {
    const r = Math.round(rgba[0] * 255).toString(16).padStart(2, '0');
    const g = Math.round(rgba[1] * 255).toString(16).padStart(2, '0');
    const b = Math.round(rgba[2] * 255).toString(16).padStart(2, '0');
    const a = Math.round(rgba[3] * 255).toString(16).padStart(2, '0');
    return `#${r}${g}${b}${a}`;
}

const animatedVarsProp = {
    set: (instance: shell.breeze_ui.js_text_widget, value: string[]) => {
        for (const v of value) {
            instance.set_animation(v, true);
        }
        // @ts-ignore
        instance._last_animated_vars = value;
    },
    get: (instance: shell.breeze_ui.js_text_widget) => {
        // @ts-ignore
        return instance._last_animated_vars;
    }
}

const componentMap = {
    text: {
        creator: shell.breeze_ui.widgets_factory.create_text_widget,
        props: {
            text: {
                set: (instance: shell.breeze_ui.js_text_widget, value: string[] | string) => {
                    instance.text = ((Array.isArray(value)) ? value.join('') : value);
                },
                get: (instance: shell.breeze_ui.js_text_widget) => {
                    return instance.text;
                }
            },
            fontSize: getSetFactory('font_size'),
            color: getSetFactoryColor('color'),
            animatedVars: animatedVarsProp
        }
    },
    flex: {
        creator: shell.breeze_ui.widgets_factory.create_flex_layout_widget,
        props: {
            padding: getSetFactoryAutoRepeat('padding'),
            paddingTop: getSetFactory('padding_top'),
            paddingRight: getSetFactory('padding_right'),
            paddingBottom: getSetFactory('padding_bottom'),
            paddingLeft: getSetFactory('padding_left'),
            onClick: getSetFactory('on_click'),
            onMouseEnter: getSetFactory('on_mouse_enter'),
            onMouseLeave: getSetFactory('on_mouse_leave'),
            onMouseDown: getSetFactory('on_mouse_down'),
            onMouseUp: getSetFactory('on_mouse_up'),
            onMouseMove: getSetFactory('on_mouse_move'),
            backgroundColor: getSetFactoryColor('background_color'),
            borderColor: getSetFactoryColor('border_color'),
            borderRadius: getSetFactory('border_radius'),
            borderWidth: getSetFactory('border_width'),
            backgroundPaint: getSetFactory('background_paint'),
            borderPaint: getSetFactory('border_paint'),
            horizontal: getSetFactory('horizontal'),
            animatedVars: animatedVarsProp,
            x: getSetFactory('x'),
            y: getSetFactory('y'),
            width: getSetFactory('width'),
            height: getSetFactory('height'),
            autoSize: getSetFactory('auto_size')
        }
    }
}

// Host config type parameters
type Type = keyof typeof componentMap;
type Props = any;
type RootContainer = shell.breeze_ui.js_flex_layout_widget;
type Instance = shell.breeze_ui.js_widget;
type TextInstance = shell.breeze_ui.js_text_widget;
type HostContext = {};
type ChildSet = void;
type HostComponent = shell.breeze_ui.js_widget

const HostConfig: Reconciler.HostConfig<
    Type,
    Props,
    RootContainer,
    Instance,
    TextInstance,
    void, // SuspenseInstance
    void, // HydratableInstance
    Instance, //PublicInstance
    HostContext,
    object, // UpdatePayload
    ChildSet,
    number, // TimeoutHandle
    -1 // NoTimeout
> = {
    getPublicInstance(instance: Instance | TextInstance) {
        return instance as Instance;
    },

    getRootHostContext(_rootContainer: RootContainer) {
        return null;
    },

    getChildHostContext(
        parentHostContext: HostContext,
        _type: Type,
        _rootContainer: RootContainer
    ): HostContext {
        return parentHostContext;
    },

    prepareForCommit(_containerInfo: RootContainer): Record<string, any> | null {
        return null;
    },

    resetAfterCommit(rootContainer: RootContainer): void {

    },

    createInstance(
        type: Type,
        props: Props,
        _rootContainer: RootContainer,
        _hostContext: HostContext,
        _internalHandle: any
    ): HostComponent {
        try {
            if (!componentMap[type]) {
                throw new Error(`Unknown component type: ${type}`);
            }
            const instance = componentMap[type].creator();
            for (const key in props) {
                if (key === 'children') {
                    continue;
                }
                const propSetter = componentMap[type]?.props?.[key];
                if (propSetter) {
                    propSetter.set(instance, props[key]);
                } else {
                    throw new Error(`Unknown property: ${key} for component type: ${type}`);
                }
            }
            return instance;
        } catch (e) {
            console.error(`Error creating instance of type ${type}:`, e, e.stack);
            throw e;
        }

    },
    appendInitialChild(parentInstance: Instance, child: Instance | TextInstance): void {
        parentInstance.append_child(child);
    },

    finalizeInitialChildren(
        _instance: Instance,
        _type: Type,
        _props: Props,
        _rootContainer: RootContainer,
        _hostContext: HostContext
    ): boolean {
        return false;
    },

    prepareUpdate(
        _instance: Instance,
        _type: Type,
        _oldProps: Props,
        newProps: Props,
        _rootContainer: RootContainer,
        _hostContext: HostContext
    ): object | null {
        const updates: Record<string, any> = {};
        for (const key in newProps) {
            if (newProps[key] !== _oldProps[key]) {
                updates[key] = newProps[key];
            }
        }
        return Object.keys(updates).length > 0 ? updates : null;
    },

    shouldSetTextContent(type: Type, props: Props): boolean {
        return false;
    },
    createTextInstance(
        text: string,
        _rootContainer: RootContainer,
        _hostContext: HostContext,
        _internalHandle: any
    ) {
        const w = shell.breeze_ui.widgets_factory.create_text_widget();
        w.text = text;
        return w;
    },

    scheduleTimeout: setTimeout,
    cancelTimeout: clearTimeout,
    noTimeout: -1,
    isPrimaryRenderer: true,
    warnsIfNotActing: true,
    supportsMutation: true,
    supportsPersistence: false,
    supportsHydration: false,

    getInstanceFromNode(_node: any) {
        throw new Error(`getInstanceFromNode not implemented`);
    },

    beforeActiveInstanceBlur() { },
    afterActiveInstanceBlur() { },

    preparePortalMount(_rootContainer: RootContainer) {
        throw new Error(`preparePortalMount not implemented`);
    },

    prepareScopeUpdate(_scopeInstance: any, _instance: any) {
        throw new Error(`prepareScopeUpdate not implemented`);
    },

    getInstanceFromScope(_scopeInstance) {
        throw new Error(`getInstanceFromScope not implemented`);
    },

    getCurrentEventPriority(): Reconciler.Lane {
        // return DefaultEventPriority;
        return 16; // DefaultEventPriority
    },

    detachDeletedInstance(_node: Instance): void { },


    commitMount(
        _instance: Instance,
        _type: Type,
        _newProps: Props,
        _internalHandle: any
    ): void {
        // This is called after the instance is mounted
    },



    commitUpdate(
        instance: Instance,
        updatePayload: object | null,
        type: Type,
        oldProps: Props,
        newProps: Props,
        internalHandle: any
    ): void {
        for (const key in newProps) {
            if (key === 'children') {
                continue;
            }

            const propSetter = componentMap[type].props[key];
            if (propSetter && newProps[key] !== oldProps[key]) {
                propSetter.set(instance, newProps[key]);
            }
        }
    },

    clearContainer(container) {
        for (const child of container.children()) {
            container.remove_child(child);
        }
    },
    appendChild(
        parentInstance: Instance,
        child: Instance | TextInstance
    ): void {
        parentInstance.append_child(child);
    },
    appendChildToContainer(
        container: RootContainer,
        child: Instance | TextInstance
    ): void {
        container.append_child(child);
    },
    removeChild(
        parentInstance: Instance,
        child: Instance | TextInstance
    ): void {
        parentInstance.remove_child(child);
    },

    removeChildFromContainer(
        container: RootContainer,
        child: Instance | TextInstance
    ): void {
        container.remove_child(child);
    },

    commitTextUpdate(
        textInstance: TextInstance,
        oldText: string,
        newText: string
    ): void {
        textInstance.text = newText;
    },

    insertBefore(parentInstance, child, beforeChild) {
        if (beforeChild) {
            parentInstance.append_child_after(child,
                parentInstance.children().indexOf(beforeChild));
        } else {
            parentInstance.append_child(child);
        }
    },

    resetTextContent(instance: Instance): void {
        const text_w = instance.downcast();
        if ('set_text' in text_w) {
            // @ts-ignore
            text_w.set_text('');
        }
    }
};


const reconciler = Reconciler(HostConfig);

export const createRenderer = (host: shell.breeze_ui.js_flex_layout_widget) => {
    return {
        render: (element: React.ReactElement) => {
            const container = reconciler.createContainer(
                host,
                0,
                null, // hydrationCallbacks
                false, // isStrictMode
                null, // concurrentUpdatesByDefaultOverride
                '',   // identifierPrefix
                (error) => console.error(error), // onRecoverableError
                null  // transitionCallbacks
            );
            reconciler.updateContainer(element, container, null, null);
        }
    };
};
