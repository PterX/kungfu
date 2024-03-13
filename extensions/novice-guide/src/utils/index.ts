/**
* 在文档中添加一条样式表规则（这可能是动态改变 class 名的更好的实现方法，
* 使得 style 样式内容可以保留在真正的样式表中，以便添加额外的元素到 DOM 中）。
* 注意这里有必要声明一个数组，因为 ECMAScript 不保证对象按预想的顺序遍历，
* 并且 CSS 也是依赖于顺序的。
* 类型为数组的参数 decls 接受一个 JSON 编译的数组。
* @example
addStylesheetRules([
 ['h2',
   ['color', 'red'],
   ['background-color', 'green', true] // 'true' for !important rules
 ],
 ['.myClass',
   ['background-color', 'yellow']
 ]
]);
*/

export type Rule = [string, string] | [string, string, boolean];
export type Rules = Rule[];
export type Decl = [string, ...Rules];
export function addStylesheetRules(decls: Decl[]) {
  const style = document.createElement('style');
  document.getElementsByTagName('head')[0].appendChild(style);

  const s = document.styleSheets[document.styleSheets.length - 1];
  for (let i = 0, dl = decls.length; i < dl; i++) {
    let j = 1,
      rulesStr = '';
    const decl = decls[i];
    const selector = decl[0];
    for (let rl = decl.length; j < rl; j++) {
      const rule = decl[j];
      rulesStr +=
        rule[0] + ':' + rule[1] + (rule[2] ? ' !important' : '') + ';\n';
    }

    s.insertRule(selector + '{' + rulesStr + '}', s.cssRules.length);
  }
}
