#set has_constructor = False
#if $current_class.methods.has_key('constructor')
#set has_constructor = True
${current_class.methods.constructor.generate_code($current_class)}
#end if

#set generator = $current_class.generator
#set methods = $current_class.methods_clean()
#set st_methods = $current_class.static_methods_clean()
#if len($current_class.parents) > 0
extern JSObject *jsb_${current_class.parents[0].underlined_class_name}_prototype;
#end if

void js_${generator.prefix}_${current_class.class_name}_finalize(JSFreeOp *fop, JSObject *obj) {
    CCLOGINFO("jsbindings: finalizing JS object %p (${current_class.class_name})", obj);
}

#if $generator.in_listed_extend_classed($current_class.class_name) and not $current_class.is_abstract
static bool js_${current_class.underlined_class_name}_ctor(JSContext *cx, uint32_t argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
    ${current_class.namespaced_class_name} *nobj = new (std::nothrow) ${current_class.namespaced_class_name}();
    if (nobj) {
        nobj->autorelease();
    }
    js_proxy_t* p = jsb_new_proxy(nobj, obj);
#if not $generator.script_control_cpp
    AddNamedObjectRoot(cx, &p->obj, "${current_class.namespaced_class_name}");
#end if
    bool isFound = false;
    if (JS_HasProperty(cx, obj, "_ctor", &isFound) && isFound)
        ScriptingCore::getInstance()->executeFunctionWithOwner(OBJECT_TO_JSVAL(obj), "_ctor", args);
    args.rval().setUndefined();
    return true;
}
#end if
\#if defined(MOZJS_MAJOR_VERSION)
\#if MOZJS_MAJOR_VERSION >= 33
void js_register_${generator.prefix}_${current_class.class_name}(JSContext *cx, JS::HandleObject global) {
\#if MOZJS_MAJOR_VERSION < 52
    jsb_${current_class.underlined_class_name}_class = (JSClass *)calloc(1, sizeof(JSClass));
    jsb_${current_class.underlined_class_name}_class->name = "${current_class.target_class_name}";
    jsb_${current_class.underlined_class_name}_class->addProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->delProperty = JS_DeletePropertyStub;
    jsb_${current_class.underlined_class_name}_class->getProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->setProperty = JS_StrictPropertyStub;
    jsb_${current_class.underlined_class_name}_class->enumerate = JS_EnumerateStub;
    jsb_${current_class.underlined_class_name}_class->resolve = JS_ResolveStub;
    jsb_${current_class.underlined_class_name}_class->convert = JS_ConvertStub;
    jsb_${current_class.underlined_class_name}_class->finalize = js_${generator.prefix}_${current_class.class_name}_finalize;
    jsb_${current_class.underlined_class_name}_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);
\#else
    static JSClass ${current_class.target_class_name}_class = {
        "${current_class.target_class_name}",
        JSCLASS_HAS_PRIVATE,
        nullptr
    };
    jsb_${current_class.underlined_class_name}_class = &${current_class.target_class_name}_class;
\#endif

    static JSPropertySpec properties[] = {
        JS_PS_END
    };

    static JSFunctionSpec funcs[] = {
        #for m in methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
#if $generator.in_listed_extend_classed($current_class.class_name) and not $current_class.is_abstract
        JS_FN("ctor", js_${current_class.underlined_class_name}_ctor, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
#end if
        JS_FS_END
    };

    #if len(st_methods) > 0
    static JSFunctionSpec st_funcs[] = {
        #for m in st_methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
        JS_FS_END
    };
    #else
    JSFunctionSpec *st_funcs = NULL;
    #end if

#if len($current_class.parents) > 0
    JS::RootedObject parent_proto(cx, jsb_${current_class.parents[0].underlined_class_name}_prototype);
#else
    JS::RootedObject parent_proto(cx, nullptr);
#end if
    JSObject* objProto = JS_InitClass(
        cx, global,
        parent_proto,
        jsb_${current_class.underlined_class_name}_class,
#if has_constructor
        js_${generator.prefix}_${current_class.class_name}_constructor, 0, // constructor
#else if $current_class.is_abstract
        empty_constructor, 0,
#else
        dummy_constructor<${current_class.namespaced_class_name}>, 0, // no constructor
#end if
        properties,
        funcs,
        NULL, // no static properties
        st_funcs);

    JS::RootedObject proto(cx, objProto);
\#if (SDKBOX_COCOS_JSB_VERSION >= 2)
\#if MOZJS_MAJOR_VERSION >= 52
    jsb_register_class<${current_class.namespaced_class_name}>(cx, jsb_${current_class.underlined_class_name}_class, proto);
\#else
    jsb_register_class<${current_class.namespaced_class_name}>(cx, jsb_${current_class.underlined_class_name}_class, proto, JS::NullPtr());
\#endif
\#else
    TypeTest<${current_class.namespaced_class_name}> t;
    js_type_class_t *p;
    std::string typeName = t.s_name();
    if (_js_global_type_map.find(typeName) == _js_global_type_map.end())
    {
        p = (js_type_class_t *)malloc(sizeof(js_type_class_t));
        p->jsclass = jsb_${current_class.underlined_class_name}_class;
        p->proto = objProto;
        p->parentProto = NULL;
        _js_global_type_map.insert(std::make_pair(typeName, p));
    }
\#endif

    // add the proto and JSClass to the type->js info hash table
    JS::RootedValue className(cx);
    JSString* jsstr = JS_NewStringCopyZ(cx, "${current_class.target_class_name}");
    className = JS::StringValue(jsstr);
    JS_SetProperty(cx, proto, "_className", className);
    JS_SetProperty(cx, proto, "__nativeObj", JS::TrueHandleValue);
    JS_SetProperty(cx, proto, "__is_ref", JS::FalseHandleValue);
}
\#else
void js_register_${generator.prefix}_${current_class.class_name}(JSContext *cx, JSObject *global) {
    jsb_${current_class.underlined_class_name}_class = (JSClass *)calloc(1, sizeof(JSClass));
    jsb_${current_class.underlined_class_name}_class->name = "${current_class.target_class_name}";
    jsb_${current_class.underlined_class_name}_class->addProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->delProperty = JS_DeletePropertyStub;
    jsb_${current_class.underlined_class_name}_class->getProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->setProperty = JS_StrictPropertyStub;
    jsb_${current_class.underlined_class_name}_class->enumerate = JS_EnumerateStub;
    jsb_${current_class.underlined_class_name}_class->resolve = JS_ResolveStub;
    jsb_${current_class.underlined_class_name}_class->convert = JS_ConvertStub;
    jsb_${current_class.underlined_class_name}_class->finalize = js_${generator.prefix}_${current_class.class_name}_finalize;
    jsb_${current_class.underlined_class_name}_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);

    static JSPropertySpec properties[] = {
        {"__nativeObj", 0, JSPROP_ENUMERATE | JSPROP_PERMANENT, JSOP_WRAPPER(js_is_native_obj), JSOP_NULLWRAPPER},
        {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
    };

    static JSFunctionSpec funcs[] = {
        #for m in methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
#if $generator.in_listed_extend_classed($current_class.class_name) and not $current_class.is_abstract
        JS_FN("ctor", js_${current_class.underlined_class_name}_ctor, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
#end if
        JS_FS_END
    };

    #if len(st_methods) > 0
    static JSFunctionSpec st_funcs[] = {
        #for m in st_methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
        JS_FS_END
    };
    #else
    JSFunctionSpec *st_funcs = NULL;
    #end if

    jsb_${current_class.underlined_class_name}_prototype = JS_InitClass(
        cx, global,
#if len($current_class.parents) > 0
        jsb_${current_class.parents[0].underlined_class_name}_prototype,
#else
        NULL, // parent proto
#end if
        jsb_${current_class.underlined_class_name}_class,
#if has_constructor
        js_${generator.prefix}_${current_class.class_name}_constructor, 0, // constructor
#else if $current_class.is_abstract
        empty_constructor, 0,
#else
        dummy_constructor<${current_class.namespaced_class_name}>, 0, // no constructor
#end if
        properties,
        funcs,
        NULL, // no static properties
        st_funcs);
    // make the class enumerable in the registered namespace
//  bool found;
//FIXME: Removed in Firefox v27
//  JS_SetPropertyAttributes(cx, global, "${current_class.target_class_name}", JSPROP_ENUMERATE | JSPROP_READONLY, &found);

    // add the proto and JSClass to the type->js info hash table
    TypeTest<${current_class.namespaced_class_name}> t;
    js_type_class_t *p;
    std::string typeName = t.s_name();
    if (_js_global_type_map.find(typeName) == _js_global_type_map.end())
    {
        p = (js_type_class_t *)malloc(sizeof(js_type_class_t));
        p->jsclass = jsb_${current_class.underlined_class_name}_class;
        p->proto = jsb_${current_class.underlined_class_name}_prototype;
#if len($current_class.parents) > 0
        p->parentProto = jsb_${current_class.parents[0].underlined_class_name}_prototype;
#else
        p->parentProto = NULL;
#end if
        _js_global_type_map.insert(std::make_pair(typeName, p));
    }
}
\#endif
\#elif defined(JS_VERSION)
void js_register_${generator.prefix}_${current_class.class_name}(JSContext *cx, JSObject *global) {
    jsb_${current_class.underlined_class_name}_class = (JSClass *)calloc(1, sizeof(JSClass));
    jsb_${current_class.underlined_class_name}_class->name = "${current_class.target_class_name}";
    jsb_${current_class.underlined_class_name}_class->addProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->delProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->getProperty = JS_PropertyStub;
    jsb_${current_class.underlined_class_name}_class->setProperty = JS_StrictPropertyStub;
    jsb_${current_class.underlined_class_name}_class->enumerate = JS_EnumerateStub;
    jsb_${current_class.underlined_class_name}_class->resolve = JS_ResolveStub;
    jsb_${current_class.underlined_class_name}_class->convert = JS_ConvertStub;
    jsb_${current_class.underlined_class_name}_class->finalize = js_${generator.prefix}_${current_class.class_name}_finalize;
    jsb_${current_class.underlined_class_name}_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);

    #if len($current_class.fields) > 0
    static JSPropertySpec properties[] = {
        {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
    };
    #else
    JSPropertySpec *properties = NULL;
    #end if

    #if len(methods) > 0
    static JSFunctionSpec funcs[] = {
        #for m in methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
#if not $current_class.is_abstract
        JS_FN("ctor", js_${generator.prefix}_${current_class.class_name}_ctor, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
#end if
        JS_FS_END
    };
    #else
    JSFunctionSpec *funcs = NULL;
    #end if

    #if len(st_methods) > 0
    static JSFunctionSpec st_funcs[] = {
        #for m in st_methods
        #set fn = m['impl']
        JS_FN("${m['name']}", ${fn.signature_name}, ${fn.min_args}, JSPROP_PERMANENT | JSPROP_ENUMERATE),
        #end for
        JS_FS_END
    };
    #else
    JSFunctionSpec *st_funcs = NULL;
    #end if

    jsb_${current_class.underlined_class_name}_prototype = JS_InitClass(
        cx, global,
#if len($current_class.parents) > 0
        jsb_${current_class.parents[0].class_name}_prototype,
#else
        NULL, // parent proto
#end if
        jsb_${current_class.underlined_class_name}_class,
#if has_constructor
        js_${generator.prefix}_${current_class.class_name}_constructor, 0, // constructor
#else if $current_class.is_abstract
        empty_constructor, 0,
#else
        dummy_constructor<${current_class.namespaced_class_name}>, 0, // no constructor
#end if
        properties,
        funcs,
        NULL, // no static properties
        st_funcs);
    // make the class enumerable in the registered namespace
    JSBool found;
    JS_SetPropertyAttributes(cx, global, "${current_class.target_class_name}", JSPROP_ENUMERATE | JSPROP_READONLY, &found);

    // add the proto and JSClass to the type->js info hash table
    TypeTest<${current_class.namespaced_class_name}> t;
    js_type_class_t *p;
    uint32_t typeId = t.s_id();
    HASH_FIND_INT(_js_global_type_ht, &typeId, p);
    if (!p) {
        p = (js_type_class_t *)malloc(sizeof(js_type_class_t));
        p->type = typeId;
        p->jsclass = jsb_${current_class.underlined_class_name}_class;
        p->proto = jsb_${current_class.underlined_class_name}_prototype;
#if len($current_class.parents) > 0
        p->parentProto = jsb_${current_class.parents[0].class_name}_prototype;
#else
        p->parentProto = NULL;
#end if
        HASH_ADD_INT(_js_global_type_ht, type, p);
    }
}
\#endif
