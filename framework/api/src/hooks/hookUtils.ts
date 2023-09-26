//返回所有category, group, name, mode用'_'拼接以及用‘*’占位的组合
export const generateCombinations = (originalKeys: string[]) => {
  const length = originalKeys.length;
  const comboLength = originalKeys.length ** 2;
  const combinations: string[] = [];
  for (let i = 0; i < comboLength; i++) {
    const binary = i.toString(2).padStart(length, '0');
    const combo: string[] = [];
    for (let j = 0; j < length; j++) {
      combo.push(binary[j] === '1' ? originalKeys[j] : '*');
    }
    combinations.push(combo.join('_'));
  }
  return combinations;
};

export const generateLocationCombinations = (
  loactionPairs: [string, string, string, string],
) => generateCombinations(loactionPairs);
