import csv

def fill_linear_mapping_from_csv_with_start_index(csv_file, size, start_index):
    filled_array = [None] * size

    # Read CSV and extract values for indexes and data
    indexes = []
    values = []
    with open(csv_file, 'r') as file:
        csv_reader = csv.reader(file)
        for row in csv_reader:
            index = int(row[0])
            if index >= start_index:
                indexes.append(index)
                values.append(float(row[1]))

    # Fill values at specified indexes
    for i in range(len(values)):
        filled_array[indexes[i]] = values[i]

    # Fill in the remaining indexes using linear mapping starting from the start_index
    for i in range(start_index, size):
        if filled_array[i] is None:
            # Find the nearest filled indexes
            left_index = None
            right_index = None
            for j in range(i - 1, start_index - 1, -1):
                if filled_array[j] is not None:
                    left_index = j
                    break
            for j in range(i + 1, size):
                if filled_array[j] is not None:
                    right_index = j
                    break
            
            # Perform linear mapping
            if left_index is not None and right_index is not None:
                left_value = filled_array[left_index]
                right_value = filled_array[right_index]
                filled_array[i] = round(left_value + (right_value - left_value) * ((i - left_index) / (right_index - left_index)))

    return filled_array

# Example usage:
csv_file = "data.csv"
size = 167
start_index = 0
result = fill_linear_mapping_from_csv_with_start_index(csv_file, size, start_index)
print(result)
