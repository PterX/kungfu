export const generateCombinations = (originalKeys: string[]) => {
  const length = originalKeys.length;
  const comboLength = originalKeys.length ** 2;
  const combinations: string[] = [];
  for (let i = 0; i < comboLength; i++) {
    const binary = i.toString(2).padStart(length, '0');
    const combo: string[] = [];
    for (let j = 0; j < 4; j++) {
      combo.push(binary[j] === '1' ? originalKeys[j] : '*');
    }
    combinations.push(combo.join('_'));
  }
  return combinations;
};
