export interface ClangASTD {
    id: string;
    kind: string;
    loc: ExpansionLOCClass;
    range: ArrayFillerRange;
    isImplicit?: boolean;
    name?: string;
    tagUsed?: string;
    inner?: ClangASTD[];
    type?: Type;
    language?: string;
    hasBraces?: boolean;
    isUsed?: boolean;
    mangledName?: string;
    isReferenced?: boolean;
    previousDecl?: string;
    originalNamespace?: OriginalNamespace;
    target?: OriginalNamespace;
    storageClass?: string;
    inline?: boolean;
    constexpr?: boolean;
    completeDefinition?: boolean;
    definitionData?: DefinitionData;
    visibility?: string;
    ownedTagDecl?: OriginalNamespace;
    numElements?: number;
    message?: string;
    depth?: number;
    index?: number;
    isParameterPack?: boolean;
    inherited?: boolean;
    virtual?: boolean;
    hasInClassInitializer?: boolean;
    parentDeclContextId?: string;
    mutable?: boolean;
    cc?: string;
    const?: boolean;
    volatile?: boolean;
    refQualifier?: string;
    exceptionSpec?: string;
    variadic?: boolean;
    isExpr?: boolean;
    usesADL?: boolean;
    lookups?: OriginalNamespace[];
    explicitlyDeleted?: boolean;
    access?: string;
    isGlobal?: boolean;
    isPlacement?: boolean;
    initStyle?: string;
    operatorNewDecl?: OriginalNamespace;
    operatorDeleteDecl?: OriginalNamespace;
    storageDuration?: string;
    boundToLValueRef?: boolean;
    path?: Path[];
    pack_index?: number;
    list?: boolean;
    zeroing?: boolean;
    hadMultipleCandidates?: boolean;
    constructionKind?: string;
    isInline?: boolean;
    immediate?: boolean;
    scopedEnumTag?: string;
    array_filler?: ArrayFiller[];
    computeLHSType?: ComputeLhsType;
    computeResultType?: ComputeLhsType;
    isPack?: boolean;
    isPostfix?: boolean;
    canOverflow?: boolean;
    isArrow?: boolean;
    referencedMemberDecl?: string;
    referencedDecl?: OriginalNamespace;
    bases?: any[];
    init?: string;
    explicitlyDefaulted?: string;
    valueCategory?: string;
    value?: string | number | boolean;
    opcode?: string;
    decl?: OriginalNamespace;
    fixedUnderlyingType?: Type;
    baseInit?: Type;
    anyInit?: OriginalNamespace;
    temp?: string;
    dtor?: OriginalNamespace;
    conversionFunc?: OriginalNamespace;
    member?: string;
    hasElse?: boolean;
    isConstexpr?: boolean;
    defaultArg?: DefaultArg;
    hasExplicitTemplateArgs?: boolean;
    explicitTemplateArgs?: ExplicitTemplateArg[];
    argType?: Type;
    hasTemplateKeyword?: boolean;
}

export interface DefinitionData {
    canPassInRegisters?: boolean;
    copyAssign?: Copy;
    copyCtor?: Copy;
    defaultCtor?: DefaultCtor;
    dtor?: Dtor;
    hasConstexprNonCopyMoveConstructor?: boolean;
    isAggregate?: boolean;
    isLiteral?: boolean;
    isPOD?: boolean;
    isStandardLayout?: boolean;
    isTrivial?: boolean;
    isTriviallyCopyable?: boolean;
    moveAssign?: Dtor;
    moveCtor?: Dtor;
    isEmpty?: boolean;
    isGenericLambda?: boolean;
    isLambda?: boolean;
    hasUserDeclaredConstructor?: boolean;
    hasVariantMembers?: boolean;
    isPolymorphic?: boolean;
    hasMutableFields?: boolean;
    isAbstract?: boolean;
}

export interface Copy {
    hasConstParam?: boolean;
    implicitHasConstParam?: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
}

export interface DefaultCtor {
    defaultedIsConstexpr?: boolean;
    exists?: boolean;
    isConstexpr?: boolean;
    needsImplicit?: boolean;
    trivial?: boolean;
    nonTrivial?: boolean;
    userProvided?: boolean;
}

export interface Dtor {
    irrelevant?: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    exists?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
    userDeclared?: boolean;
    hasConstParam?: boolean;
    implicitHasConstParam?: boolean;
}

export interface OriginalNamespace {
    id: string;
    kind?: string;
    name?: string;
    type?: Type;
}

export interface Type {
    desugaredQualType?: string;
    qualType: string;
    typeAliasDeclId?: string;
}

export interface ArrayFiller {
    id: string;
    kind: string;
    range: ArrayFillerRange;
    type: ComputeLhsType;
    valueCategory: string;
}

export interface ArrayFillerRange {
    begin: ExpansionLOCClass;
    end: ExpansionLOCClass;
}

export interface ExpansionLOCClass {
    offset?: number;
    col?: number;
    tokLen?: number;
    includedFrom?: IncludedFrom;
    line?: number;
    file?: string;
    spellingLoc?: ExpansionLOCClass;
    expansionLoc?: ExpansionLOCClass;
    isMacroArgExpansion?: boolean;
    presumedLine?: number;
}

export interface IncludedFrom {
    file: string;
}

export interface Path {
    name: string;
}

export interface ComputeLhsType {
    qualType: string;
}

export interface DefaultArg {
    kind: string;
    type?: Type;
    isExpr?: boolean;
    "inherited from"?: OriginalNamespace;
}

export interface ExplicitTemplateArg {
    kind: string;
    range: ExplicitTemplateArgRange;
    isExpr: boolean;
}

export interface ExplicitTemplateArgRange {
    begin: ExpansionLOCClass;
    end: ExpansionLOCClass;
}
